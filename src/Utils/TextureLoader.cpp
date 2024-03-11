#include "./TextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../Buffer.h"
#include "../BufferHelper.h"
#include "../LogicalDevice.h"
#include "../PhysicalDevice.h"
#include "../CommandPool.h"
#include "../CommandBuffer.h"

namespace Engine {
	namespace Utils {
		TextureLoader::TextureLoader() {

		}

		TextureLoader::~TextureLoader() {

		}

		void TextureLoader::LoadTexture(
			std::unique_ptr<Image>& texture, 
			const char* texturePath, 
			LogicalDevice& logicalDevice, 
			PhysicalDevice& physicalDevice, 
			CommandPool& commandPool,
			bool flipTextureVertically
		) {
			int texWidth = 0;
			int texHeight = 0;
			int texChannels = 0;

			stbi_set_flip_vertically_on_load(flipTextureVertically);
			stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			VkDeviceSize imageSize = texWidth * texHeight * 4;

			if (!pixels) {
				throw std::runtime_error("Failed to load texture image!");
			}

			Buffer transferBuffer = Buffer(
				1, 
				logicalDevice, 
				physicalDevice, 
				imageSize, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			);

			transferBuffer.AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			BufferHelper bufferHelper;
			bufferHelper.CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(), pixels, imageSize, &transferBuffer);

			stbi_image_free(pixels);

			texture = std::make_unique<class Image> (
				1,
				logicalDevice.GetHandle(),
				physicalDevice.GetHandle(),
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight),
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_ASPECT_COLOR_BIT	
			);

			texture->CreateImageView();

			texture->TransitionImageLayoutTo(
				commandPool.GetHandle(),
				logicalDevice.GetGraphicsQueue(),
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);

			Buffer::CopyToImage(
				logicalDevice.GetHandle(),
				commandPool.GetHandle(),
				logicalDevice.GetGraphicsQueue(),
				texture->GetImage(0),
				texture->Width,
				texture->Height,
				texture->ImageLayout,
				transferBuffer.GetBuffer(0)
			);

			texture->TransitionImageLayoutTo(
				commandPool.GetHandle(),
				logicalDevice.GetGraphicsQueue(),
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			texture->CreateImageSampler();
		}

		Assets::Texture TextureLoader::CreateTexture(
			Assets::TextureType textureType, 
			const char* texturePath, 
			LogicalDevice& logicalDevice,
			PhysicalDevice& physicalDevice, 
			CommandPool& commandPool,
			bool flipTextureVertically) {
		
			Assets::Texture texture;

			TextureLoader::LoadTexture(
				texture.TextureImage, 
				texturePath, 
				logicalDevice, 
				physicalDevice, 
				commandPool, 
				flipTextureVertically
			);

			return texture;
		}
	}
}