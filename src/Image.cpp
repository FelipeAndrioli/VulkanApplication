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
		const VkImageAspectFlags aspectFlags
	): 
		p_LogicalDevice(&logicalDevice), 
		p_PhysicalDevice(&physicalDevice),
		m_Format(format), 
		m_Tiling(tiling), 
		m_Usage(usage), 
		m_Properties(properties), 
		m_ImageSize(imageSize),
		m_AspectFlags(aspectFlags),
		m_ImageWidth(imageWidth),
		m_ImageHeight(imageHeight) {

		m_ImageMemory.reset(new class DeviceMemory(p_LogicalDevice, p_PhysicalDevice, m_ImageSize));

		CreateImage();
	}
	
	Image::~Image() {
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
	
		m_ImageWidth = imageWidth;
		m_ImageHeight = imageHeight;

		m_ImageMemory.reset(new class DeviceMemory(p_LogicalDevice, p_PhysicalDevice, m_ImageSize));
		
		CreateImage();
		CreateImageView();
	}

	void Image::CreateImage() {
		m_Image.resize(m_ImageSize);

		for (size_t i = 0; i < m_Image.size(); i++) {
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.extent.width = m_ImageWidth;
			imageCreateInfo.extent.height = m_ImageHeight;
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
		for (VkImage image : m_Image) {
			vkDestroyImage(*p_LogicalDevice, image, nullptr);
		}

		for (VkImageView imageView : ImageView) {
			vkDestroyImageView(*p_LogicalDevice, imageView, nullptr);
		}

		m_Image.clear();
		ImageView.clear();
		m_ImageMemory.reset();
	}
}