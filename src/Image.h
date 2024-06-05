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
			const VkImageAspectFlags aspectFlags, 
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
		);
		~Image();
		
		VkImage& GetImage() { return m_Image; }

		void CreateImageView(const VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
			const VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
			const uint32_t layerCount = 1
		);
		void Resize(const uint32_t imageWidth, const uint32_t imageHeight);
		void TransitionImageLayoutTo(
			VkCommandPool& commandPool,
			VkQueue& queue,
			VkImageLayout newLayout,
			uint32_t layerCount = 1
		);
		void CreateImageSampler(VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
		void GenerateMipMaps(VkCommandPool& commandPool, VkQueue& queue);
	public:
		VkImageView ImageView = VK_NULL_HANDLE;
		VkSampler ImageSampler = VK_NULL_HANDLE;
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
		VkImageViewType m_ViewType = VK_IMAGE_VIEW_TYPE_2D;
		uint32_t m_LayerCount = 1;
		
		VkDevice* p_LogicalDevice = nullptr;
		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
