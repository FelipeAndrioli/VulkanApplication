#include "DepthBuffer.h"

namespace Engine {
	DepthBuffer::DepthBuffer(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, const SwapChain& swapChain) 
	: p_PhysicalDevice(&physicalDevice) {
		VkFormat depthFormat = FindDepthFormat();

		m_DepthBufferImage.reset(new class Image(
			1, 
			logicalDevice, 
			*p_PhysicalDevice, 
			swapChain.GetSwapChainExtent().width,
			swapChain.GetSwapChainExtent().height, 
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			VK_IMAGE_ASPECT_DEPTH_BIT
		));

		m_DepthBufferImage->CreateImageView();
	}

	DepthBuffer::~DepthBuffer() {
		m_DepthBufferImage.reset();
	}

	void DepthBuffer::Resize(const uint32_t width, const uint32_t height) {
		m_DepthBufferImage->Resize(width, height);
	}

	VkFormat DepthBuffer::FindDepthFormat() {
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat DepthBuffer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
		VkFormatFeatureFlags features) {

		for (VkFormat format : candidates) {
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(*p_PhysicalDevice, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("Failed to find supported format!");
	}

	bool DepthBuffer::HasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
}