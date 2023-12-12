#pragma once

#include "Vulkan.h"
#include "DeviceMemory.h"

namespace Engine {
	class Image {
	public:
		Image(
			int imageSize, 
			VkDevice& logicalDevice, 
			VkPhysicalDevice& physicalDevice, 
			const uint32_t imageWidth, 
			const uint32_t imageHeight, 
			const VkFormat format, 
			const VkImageTiling tiling, 
			const VkImageUsageFlagBits usage, 
			const VkMemoryPropertyFlagBits properties,
			const VkImageAspectFlags aspectFlags = NULL
		);
		~Image();

		void CreateImageView();
		void Resize(const uint32_t imageWidth, const uint32_t imageHeight);
	public:
		std::vector<VkImageView> ImageView;
	private:
		void CreateImage();
		void CleanUp();
	private:
		std::vector<VkImage> m_Image;
		std::unique_ptr<class DeviceMemory> m_ImageMemory;

		VkFormat m_Format;
		VkImageTiling m_Tiling;
		VkImageUsageFlagBits m_Usage;
		VkMemoryPropertyFlagBits m_Properties;
		VkImageAspectFlags m_AspectFlags;
		
		uint32_t m_ImageSize;
		uint32_t m_ImageWidth;
		uint32_t m_ImageHeight;

		VkDevice* p_LogicalDevice = nullptr;
		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
