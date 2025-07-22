#pragma once

#include "VulkanHeader.h"

#include <string> 
#include <vector>

namespace Graphics {

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

	struct ImageDescription {
		uint32_t Width				= 0;
		uint32_t Height				= 0;
		uint32_t MipLevels			= 1;
		uint32_t LayerCount			= 1;
		uint32_t BaseArrayLayer		= 0;

		VkFormat					Format				= VK_FORMAT_R8G8B8A8_SRGB;
		VkImageTiling				Tiling				= VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlagBits		Usage				= static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_SAMPLED_BIT);
		VkMemoryPropertyFlagBits	MemoryProperty		= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		VkImageAspectFlags			AspectFlags			= VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewType				ViewType			= VK_IMAGE_VIEW_TYPE_2D;
		VkSampleCountFlagBits		MsaaSamples			= VK_SAMPLE_COUNT_1_BIT;
		VkImageType					ImageType			= VK_IMAGE_TYPE_2D;
		VkSamplerAddressMode		AddressMode			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkImageAspectFlags			AspectMask			= VK_IMAGE_ASPECT_NONE;
		VkCompareOp					SamplerCompareOp	= VK_COMPARE_OP_ALWAYS;
		VkComponentMapping			ComponentMapping	= {};
	};

	struct GPUImage {
		VkImage			Image			= VK_NULL_HANDLE;
		VkImageView		ImageView		= VK_NULL_HANDLE;
		VkSampler		ImageSampler	= VK_NULL_HANDLE;
		VkDeviceMemory	Memory			= VK_NULL_HANDLE;
		VkImageLayout	ImageLayout		= VK_IMAGE_LAYOUT_UNDEFINED;

		void*			MemoryMapped;

		ImageDescription Description = {};
	};

	struct GPUImageCube : public GPUImage {
		VkImageView ImageViews[6] = { VK_NULL_HANDLE };
	};

	struct Texture : public GPUImage {

		enum TextureType {
			AMBIENT				= 0,
			DIFFUSE				= 1,
			SPECULAR			= 2,
			SPECULAR_HIGHTLIGHT = 3,
			BUMP				= 4,
			DISPLACEMENT		= 5,
			ALPHA				= 6,
			REFLECTION			= 7,
			ROUGHNESS			= 8,
			METALLIC			= 9,
			SHEEN				= 10,
			EMISSIVE			= 11,
			NORMAL				= 12,
			CUBEMAP_SINGLE		= 13,
			CUBEMAP_MULTI		= 14,
			UNKNOWN				= 99
		} Type = TextureType::UNKNOWN;

		std::string Name = "";
	};

	struct BufferDescription {
		VkBufferUsageFlags Usage				= VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
		VkMemoryPropertyFlagBits MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		size_t Size								= 0;
		size_t Capacity							= 0;
	};

	struct GPUBuffer {
		VkBuffer Handle					= VK_NULL_HANDLE;
		VkDeviceMemory Memory			= VK_NULL_HANDLE;
		void* MemoryMapped				= nullptr;

		BufferDescription Description	= {};
	};

	struct Buffer {
		size_t Capacity		= 0;
		size_t Size			= 0;
		size_t Offset		= 0;

		VkBuffer* Handle = nullptr;
	};

	struct PipelineLayoutDesc {
		std::vector<VkDescriptorSetLayout> SetLayouts;
		std::vector<VkPushConstantRange> PushConstantRanges;
	};
}