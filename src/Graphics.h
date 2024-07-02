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
		VkSampleCountFlagBits MsaaSamples;

		VkDeviceMemory Memory;
		void* MemoryMapped;

		VkFormat Format;
		VkImageTiling Tiling;
		VkImageUsageFlagBits Usage;
		VkMemoryPropertyFlagBits Properties;
		VkImageAspectFlags AspectFlags;
		VkImageViewType ViewType = VK_IMAGE_VIEW_TYPE_2D;
		uint32_t LayerCount = 1;
	};
}