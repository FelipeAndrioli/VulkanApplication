#include "./TextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "../Buffer.h"
#include "../BufferHelper.h"
#include "../LogicalDevice.h"
#include "../PhysicalDevice.h"
#include "../CommandPool.h"
#include "../CommandBuffer.h"
#include "../Vulkan.h"

#include "./Bitmap.h"
#include "./UtilsCubemap.h"

namespace Engine {
	namespace Utils {
		TextureLoader::TextureLoader() {

		}

		TextureLoader::~TextureLoader() {

		}

		Assets::Texture TextureLoader::LoadTexture(
			const char* texturePath, 
			VulkanEngine& vulkanEngine,
			bool flipTextureVertically,
			bool generateMipMaps
		) {
			Assets::Texture texture = {};

			int texWidth = 0;
			int texHeight = 0;
			int texChannels = 0;

			stbi_set_flip_vertically_on_load(flipTextureVertically);
			stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			VkDeviceSize imageSize = texWidth * texHeight * 4;

			uint32_t mipLevels = 1;

			if (generateMipMaps)
				mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

			if (!pixels) {
				throw std::runtime_error("Failed to load texture image!");
			}

			Buffer transferBuffer = Buffer(
				1, 
				vulkanEngine,
				imageSize, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			);

			transferBuffer.AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			BufferHelper::CopyFromStaging(vulkanEngine, pixels, imageSize, &transferBuffer);

			stbi_image_free(pixels);

			texture.TextureImage = std::make_unique<class Image> (
				vulkanEngine.GetLogicalDevice().GetHandle(),
				vulkanEngine.GetPhysicalDevice().GetHandle(),
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight),
				mipLevels,
				VK_SAMPLE_COUNT_1_BIT,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_ASPECT_COLOR_BIT	
			);

			texture.TextureImage->CreateImageView();
			texture.TextureImage->TransitionImageLayoutTo(
				vulkanEngine.GetCommandPool().GetHandle(),
				vulkanEngine.GetLogicalDevice().GetGraphicsQueue(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);

			Buffer::CopyToImage(
				vulkanEngine,
				vulkanEngine.GetLogicalDevice().GetGraphicsQueue(),
				texture.TextureImage->GetImage(),
				texture.TextureImage->Width,
				texture.TextureImage->Height,
				texture.TextureImage->ImageLayout,
				transferBuffer.GetBuffer(0)
			);

			texture.TextureImage->GenerateMipMaps(vulkanEngine.GetCommandPool().GetHandle(), vulkanEngine.GetLogicalDevice().GetGraphicsQueue());
			texture.TextureImage->CreateImageSampler();
			
			return texture;
		}

		uint32_t bytesPerTexFormat(VkFormat format) {
			switch (format) {
			case VK_FORMAT_R8_SINT:
			case VK_FORMAT_R8_UNORM:
				return 1;
			case VK_FORMAT_R16_SFLOAT:
				return 2;
			case VK_FORMAT_R16G16_SFLOAT:
			case VK_FORMAT_R16G16_SNORM:
			case VK_FORMAT_B8G8R8A8_UNORM:
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_R8G8B8A8_SRGB:
				return 4;
			case VK_FORMAT_R16G16B16A16_SFLOAT:
				return 4 * sizeof(uint16_t);
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				return 4 * sizeof(float);
			default:
				break;
			}

			return 0;
		}

		static void float24to32(int w, int h, const float* img24, float* img32) {
			const int numPixels = w * h;

			for (int i = 0; i < numPixels; i++) {
				*img32++ = *img24++;
				*img32++ = *img24++;
				*img32++ = *img24++;
				*img32++ = 1.0f;
			}
		}

		Assets::Texture TextureLoader::LoadCubemapTexture(const char* texturePath, VulkanEngine& vulkanEngine) {
			Assets::Texture texture = {};
			const uint32_t mipLevels = 1;
		
			int width;
			int height;
			int comp;

			stbi_set_flip_vertically_on_load(false);

			const float* img = stbi_loadf(texturePath, &width, &height, &comp, 3);

			if (!img) {
				throw std::runtime_error("Failed to load image!");
			}

			// stbi_loadf only load 3 components, we need to convert it from float (8) 24 (RGB) float (8) to 32 (RGBA)
			// setting the fourth component to 1.0f, Vulkan doesn't have a R24G24B24 format
			std::vector<float> img32(width * height * 4);
			float24to32(width, height, img, img32.data());
			comp = 4;

			Bitmap in(width, height, comp, eBitmapFormat_Float, img32.data());
			Bitmap out = Utils::convertEquirectangularMapToVerticalCross(in);

			stbi_image_free((void*)img);

			Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);

			VkFormat texFormat = VK_FORMAT_R32G32B32A32_SFLOAT; 

			// update texture image
			uint32_t bytesPerPixel = bytesPerTexFormat(texFormat);
			uint32_t layerCount = 6;

			VkDeviceSize layerSize = cubemap.getWidth() * cubemap.getHeight() * bytesPerPixel;
			VkDeviceSize imageSize = layerSize * layerCount;

			texture.TextureImage = std::make_unique<class Image>(
				vulkanEngine.GetLogicalDevice().GetHandle(),
				vulkanEngine.GetPhysicalDevice().GetHandle(),
				static_cast<uint32_t>(cubemap.getWidth()),
				static_cast<uint32_t>(cubemap.getHeight()),
				mipLevels,
				VK_SAMPLE_COUNT_1_BIT,
				texFormat,
				VK_IMAGE_TILING_OPTIMAL,
				static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
			);
			texture.TextureImage->CreateImageView(VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, layerCount);
			texture.TextureImage->TransitionImageLayoutTo(
				vulkanEngine.GetCommandPool().GetHandle(),
				vulkanEngine.GetLogicalDevice().GetGraphicsQueue(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				layerCount
			);

			BufferHelper::UploadDataToImage(vulkanEngine, cubemap.Data.data(), imageSize, texture.TextureImage->GetImage(), 
				cubemap.getWidth(), cubemap.getHeight(), layerCount);

			texture.TextureImage->TransitionImageLayoutTo(
				vulkanEngine.GetCommandPool().GetHandle(),
				vulkanEngine.GetLogicalDevice().GetGraphicsQueue(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				layerCount
			);

			texture.TextureImage->CreateImageSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

			return texture;
		}
	}
}
