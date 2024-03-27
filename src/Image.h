#pragma once

#include "Vulkan.h"
#include "DeviceMemory.h"

namespace Engine {
	class Image {
	public:
		Image(
			VkDevice& logicalDevice, 
			VkPhysicalDevice& physicalDevice, 
			const uint32_t imageWidth, 
			const uint32_t imageHeight, 
			const uint32_t mipLevels,
			const VkSampleCountFlagBits msaaSamples,
			const VkFormat format, 
			const VkImageTiling tiling, 
			const VkImageUsageFlagBits usage, 
			const VkMemoryPropertyFlagBits properties,
			const VkImageAspectFlags aspectFlags = NULL, 
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
		);
		~Image();
		
		VkImage& GetImage() { return m_Image; }

		void CreateImageView();
		void Resize(const uint32_t imageWidth, const uint32_t imageHeight);
		void TransitionImageLayoutTo(
			VkCommandPool& commandPool,
			VkQueue& queue,
			VkFormat format,
			VkImageLayout newLayout
		);
		void CreateImageSampler();
		void GenerateMipMaps(VkCommandPool& commandPool, VkQueue& queue);
	public:
		VkImageView ImageView;
		VkSampler ImageSampler;
		VkImageLayout ImageLayout;

		uint32_t Width;
		uint32_t Height;
		uint32_t MipLevels;
		VkSampleCountFlagBits MsaaSamples;
	private:
		void CreateImage();
		void CleanUp();
	private:
		VkImage m_Image;
		std::unique_ptr<class DeviceMemory> m_ImageMemory;

		VkFormat m_Format;
		VkImageTiling m_Tiling;
		VkImageUsageFlagBits m_Usage;
		VkMemoryPropertyFlagBits m_Properties;
		VkImageAspectFlags m_AspectFlags;
		
		VkDevice* p_LogicalDevice = nullptr;
		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
