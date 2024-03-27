#include "./TextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../Buffer.h"
#include "../BufferHelper.h"
#include "../LogicalDevice.h"
#include "../PhysicalDevice.h"
#include "../CommandPool.h"
#include "../CommandBuffer.h"
#include "../Vulkan.h"

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

			BufferHelper bufferHelper;
			bufferHelper.CopyFromStaging(vulkanEngine, pixels, imageSize, &transferBuffer);

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
				VK_FORMAT_R8G8B8A8_SRGB,
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
	}
}