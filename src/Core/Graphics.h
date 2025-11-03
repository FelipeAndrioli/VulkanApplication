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

	enum class Format : uint8_t {
		UNKNOWN = 0,

		R32G32B32A32_FLOAT,
		R32G32B32A32_INT,
		R32G32B32A32_SINT,

		R32G32B32_FLOAT,
		R32G32B32_INT,
		R32G32B32_SINT,

		R16G16B16A16_FLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SNORM,
		R16G16B16A16_SINT,

//		R8G8B8A8_FLOAT,
		R8G8B8A8_UNORM,
		R8G8B8A8_UINT,
		R8G8B8A8_SNORM,
		R8G8B8A8_SINT,

		D32_FLOAT_S8_UINT,
		D24_UNORM_S8_UINT,
		D16_UNORM_S8_UINT,
		D32_FLOAT,
		D16_UNORM,
		S8_UINT,

	};

	enum class ResourceState {
		// Common resource states
		UNDEFINED				= 0,
		SHADER_RESOURCE			= 1 << 0,	// shader resource read-only
		SHADER_RESOURCE_COMPUTE = 1 << 1,	// shader resource, read only, non-pixel shader
		UNORDERED_ACCESS		= 1 << 2,	// shader resource, write enabled
		COPY_SRC				= 1 << 3,	// copy from
		COPY_DST				= 1 << 4,	// copy to

		// Texture resource states
		RENDERTARGET			= 1 << 5,	// render target, write enabled
		DEPTHSTENCIL			= 1 << 6,	// depth stencil, write enabled
		DEPTHSTENCIL_READONLY	= 1 << 7,	// depth stencil, read only

		// Buffer resource states
		VERTEX_BUFFER			= 1 << 8,	// vertex buffer, read only
		INDEX_BUFFER			= 1 << 9,	// index buffer, read only
		CONSTANT_BUFFER			= 1 << 10,	// constant buffer, read only

		// Other
		SWAPCHAIN				= 1 << 11
	};

	inline bool operator & (ResourceState a, ResourceState b) {
		return static_cast<ResourceState>(a) & static_cast<ResourceState>(b);
	}

	inline bool operator | (ResourceState a, ResourceState b) {
		return static_cast<ResourceState>(a) | static_cast<ResourceState>(b);
	}

	struct RenderPassAttachment {

		enum class AttachmentType {
			RENDERTARGET = 0,
			DEPTHSTENCIL,
			RESOLVE
		} Type = AttachmentType::RENDERTARGET;

		enum class AttachmentLoadOp {
			LOAD = 0,
			CLEAR,
			DONTCARE
		} LoadOp = AttachmentLoadOp::LOAD;

		enum class AttachmentStoreOp {
			STORE = 0,
			DONTCARE
		} StoreOp = AttachmentStoreOp::STORE;

		ResourceState InitialLayout = ResourceState::UNDEFINED;
		ResourceState SubpassLayout = ResourceState::UNDEFINED;
		ResourceState FinalLayout	= ResourceState::UNDEFINED;

		Format ImageFormat = Format::UNKNOWN;

		uint8_t SampleCount = 1;

		GPUImage Texture;

		static RenderPassAttachment RenderTarget(
			const GPUImage& resource,
			Format format,
			AttachmentLoadOp loadOp		= AttachmentLoadOp::LOAD,
			AttachmentStoreOp storeOp	= AttachmentStoreOp::STORE,
			ResourceState initialLayout = ResourceState::SHADER_RESOURCE,
			ResourceState subpassLayout = ResourceState::RENDERTARGET,
			ResourceState finalLayout	= ResourceState::SHADER_RESOURCE
		) {
			RenderPassAttachment attachment;
			attachment.Type				= AttachmentType::RENDERTARGET;
			attachment.Texture			= resource;
			attachment.LoadOp			= loadOp;
			attachment.StoreOp			= storeOp;
			attachment.InitialLayout	= initialLayout;
			attachment.SubpassLayout	= subpassLayout;
			attachment.FinalLayout		= finalLayout;
			attachment.ImageFormat		= format;
			attachment.SampleCount		= resource.Description.MsaaSamples;

			return attachment;
		}

		static RenderPassAttachment DepthStencil(
			const GPUImage& resource,
			Format format,
			AttachmentLoadOp loadOp		= AttachmentLoadOp::LOAD,
			AttachmentStoreOp storeOp	= AttachmentStoreOp::STORE,
			ResourceState initialLayout = ResourceState::DEPTHSTENCIL,
			ResourceState subpassLayout = ResourceState::DEPTHSTENCIL,
			ResourceState finalLayout	= ResourceState::DEPTHSTENCIL
		) {
			RenderPassAttachment attachment;
			attachment.Type				= AttachmentType::DEPTHSTENCIL;
			attachment.Texture			= resource;
			attachment.LoadOp			= loadOp;
			attachment.StoreOp			= storeOp;
			attachment.InitialLayout	= initialLayout;
			attachment.SubpassLayout	= subpassLayout;
			attachment.FinalLayout		= finalLayout;
			attachment.ImageFormat		= format;
			attachment.SampleCount		= resource.Description.MsaaSamples;

			return attachment;
		}

		static RenderPassAttachment Resolve(
			const GPUImage& resource,
			Format format,
			AttachmentLoadOp loadOp		= AttachmentLoadOp::LOAD,
			AttachmentStoreOp storeOp	= AttachmentStoreOp::STORE,
			ResourceState initialLayout = ResourceState::SHADER_RESOURCE,
			ResourceState subpassLayout = ResourceState::RENDERTARGET,
			ResourceState finalLayout	= ResourceState::SHADER_RESOURCE
		) {
			RenderPassAttachment attachment;
			attachment.Type				= AttachmentType::RESOLVE;
			attachment.Texture			= resource;
			attachment.LoadOp			= loadOp;
			attachment.StoreOp			= storeOp;
			attachment.InitialLayout	= initialLayout;
			attachment.SubpassLayout	= subpassLayout;
			attachment.FinalLayout		= finalLayout;
			attachment.ImageFormat		= format;
			attachment.SampleCount		= resource.Description.MsaaSamples;

			return attachment;
		}
	};

	struct RenderPassDescription {
		std::vector<RenderPassAttachment> Attachments;
		VkRenderPass Handle = VK_NULL_HANDLE;
	};

	struct SubPassDescription {
		std::vector<uint32_t> ColorAttachmentIndices;		// Indices into attachments list
		std::vector<uint32_t> ResolveAttachmentIndices;		// Parallel to ColorAttachmentsIndices
		std::vector<uint32_t> InputAttachmentIndices;		// For reading previous outputs (e.g., G-Buffers in deferred)
		std::vector<uint32_t> PreserveAttachmentIndices;	// If needed for attachments not used but preserved
		uint32_t DepthStencilAttachmentIndex = VK_ATTACHMENT_UNUSED;
	};
}