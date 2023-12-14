#include "./TextureLoader.h"

namespace Engine {
	namespace Utils {
		TextureLoader::TextureLoader() {

		}

		TextureLoader::~TextureLoader() {

		}

		void TextureLoader::LoadTexture(const char* texturePath, LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, 
			CommandPool& commandPool, VkQueue& queue) {
			int texWidth = 0;
			int texHeight = 0;
			int texChannels = 0;

			stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			VkDeviceSize imageSize = texWidth * texHeight * 4;

			if (!pixels) {
				throw std::runtime_error("Failed to load texture iamge!");
			}

			Buffer transferBuffer = Buffer(
				1, 
				logicalDevice, 
				physicalDevice, 
				imageSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			);

			transferBuffer.AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			BufferHelper bufferHelper;
			bufferHelper.CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(), pixels, imageSize, &transferBuffer);

			stbi_image_free(pixels);

			// Test if we need to return a pointer instead and cleanup the resources later
			Engine::Image texelImage = Engine::Image(
				1,
				logicalDevice.GetHandle(),
				physicalDevice.GetHandle(),
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight),
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				NULL
			);

			texelImage.CreateImageView();

			texelImage.TransitionImageLayoutTo(
				commandPool.GetHandle(),
				queue	,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);

			Buffer::CopyToImage(
				logicalDevice.GetHandle(),
				commandPool.GetHandle(),
				queue,
				texelImage.GetImage(0),
				texelImage.Width,
				texelImage.Height,
				texelImage.ImageLayout,
				transferBuffer.GetBuffer(0)
			);

			texelImage.TransitionImageLayoutTo(
				commandPool.GetHandle(),
				queue,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
	}
}