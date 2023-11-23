#pragma once

#include "Vulkan.h"
#include "DeviceMemory.h"
#include "SwapChain.h"

namespace Engine {
	class Image {
	public:
		Image(int imageSize, VkDevice* logicalDevice, VkPhysicalDevice* physicalDevice, SwapChain* swapChain, 
			VkFormat format, VkImageTiling tiling, VkImageUsageFlagBits usage, VkMemoryPropertyFlagBits properties,
			VkImageAspectFlags aspectFlags = NULL);
		~Image();

		void CreateImageView();
		void Resize(SwapChain* swapChain);
	public:
		std::vector<VkImageView> ImageView;
	private:
		void CreateImage(SwapChain* swapChain);
		void CleanUp();
	private:
		std::vector<VkImage> m_Image;
		std::unique_ptr<class DeviceMemory> m_ImageMemory;

		VkFormat m_Format;
		VkImageTiling m_Tiling;
		VkImageUsageFlagBits m_Usage;
		VkMemoryPropertyFlagBits m_Properties;
		VkImageAspectFlags m_AspectFlags;
		int m_ImageSize;

		VkDevice* p_LogicalDevice = nullptr;
		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
