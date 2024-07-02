#pragma once

#include "VulkanHeader.h"

namespace Engine::Graphics {

	struct Viewport {
		float top_left_x = 0;
		float top_left_y = 0;
		float width = 0;
		float height = 0;
		float min_depth = 0;
		float max_depth = 1;
	};

	struct Rect {
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
	};

	struct GPUImage {
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VkSampler ImageSampler = VK_NULL_HANDLE;
		VkImageLayout ImageLayout;

		uint32_t Width;
		uint32_t Height;
		uint32_t MipLevels;
		uint32_t LayerCount = 1;

		VkDeviceMemory Memory;
		void* MemoryMapped;

		VkFormat Format = VK_FORMAT_R8G8B8A8_SRGB;
		VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlagBits Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		VkMemoryPropertyFlagBits MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewType ViewType = VK_IMAGE_VIEW_TYPE_2D;
		VkSampleCountFlagBits MsaaSamples;
		VkImageType ImageType = VK_IMAGE_TYPE_2D;
	};
}