#include "Image.h"

namespace Engine {
	Image::Image(
		int imageSize, 
		VkDevice& logicalDevice, 
		VkPhysicalDevice& physicalDevice, 
		const uint32_t imageWidth, 
		const uint32_t imageHeight, 
		const VkFormat format, 
		const VkImageTiling tiling,
		const VkImageUsageFlagBits usage, 
		const VkMemoryPropertyFlagBits properties,
		const VkImageAspectFlags aspectFlags,
		VkImageLayout imageLayout
	): 
		p_LogicalDevice(&logicalDevice), 
		p_PhysicalDevice(&physicalDevice),
		m_Format(format), 
		m_Tiling(tiling), 
		m_Usage(usage), 
		m_Properties(properties), 
		m_ImageSize(imageSize),
		m_AspectFlags(aspectFlags),
		ImageLayout(imageLayout),
		Width(imageWidth),
		Height(imageHeight) {

		m_ImageMemory.reset(new class DeviceMemory(p_LogicalDevice, p_PhysicalDevice, m_ImageSize));

		CreateImage();
	}
	
	Image::~Image() {
		vkDestroySampler(*p_LogicalDevice, m_ImageSampler, nullptr);
		CleanUp();
	}

	void Image::CreateImageView() {
		ImageView.resize(m_ImageSize);

		for (size_t i = 0; i < m_Image.size(); i++) {
			VkImageViewCreateInfo viewCreateInfo{};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.image = m_Image[i];
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.format = m_Format;
			viewCreateInfo.subresourceRange.aspectMask = m_AspectFlags;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(*p_LogicalDevice, &viewCreateInfo, nullptr, &ImageView[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Image View!");
			}
		}
	}

	void Image::Resize(const uint32_t imageWidth, const uint32_t imageHeight) {
		CleanUp();
	
		Width = imageWidth;
		Height = imageHeight;

		m_ImageMemory.reset(new class DeviceMemory(p_LogicalDevice, p_PhysicalDevice, m_ImageSize));
		
		CreateImage();
		CreateImageView();
	}

	void Image::TransitionImageLayoutTo(
		VkCommandPool& commandPool,
		VkQueue& queue,
		VkFormat format,
		VkImageLayout newLayout
	) {

		for (VkImage& image : m_Image) {
			VkCommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(*p_LogicalDevice, commandPool);

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = ImageLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage;
			VkPipelineStageFlags dstStage;

			if (ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			} else if (ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} else {
				throw std::invalid_argument("Unsupported layout transition!");
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage,  
				dstStage, 
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier
			);

			CommandBuffer::EndSingleTimeCommandBuffer(*p_LogicalDevice, queue, commandBuffer, commandPool);
		}

		ImageLayout = newLayout;
	}

	void Image::CreateImageSampler() {

		// TODO: add parameters as variables
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(*p_PhysicalDevice, &properties);
	
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(*p_LogicalDevice, &samplerInfo, nullptr, &m_ImageSampler) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image sampler!");
		}
	}

	void Image::CreateImage() {
		m_Image.resize(m_ImageSize);

		for (size_t i = 0; i < m_Image.size(); i++) {
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.extent.width = Width;
			imageCreateInfo.extent.height = Height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.format = m_Format;
			imageCreateInfo.tiling = m_Tiling;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.usage = m_Usage;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateImage(*p_LogicalDevice, &imageCreateInfo, nullptr, &m_Image[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image!");
			}
		}

		m_ImageMemory->AllocateMemory(m_Image, m_Properties);
	}

	void Image::CleanUp() {
		for (VkImageView imageView : ImageView) {
			vkDestroyImageView(*p_LogicalDevice, imageView, nullptr);
		}

		for (VkImage image : m_Image) {
			vkDestroyImage(*p_LogicalDevice, image, nullptr);
		}

		ImageView.clear();
		m_Image.clear();
		m_ImageMemory.reset();
	}
}