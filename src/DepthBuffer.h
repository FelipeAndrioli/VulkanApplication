#pragma once

#include <vector>
#include <stdexcept>

#include "Vulkan.h"
#include "SwapChain.h"
#include "Image.h"

namespace Engine {
	class DepthBuffer {
	public:
		DepthBuffer(
			VkPhysicalDevice& physicalDevice, 
			VkDevice& logicalDevice, 
			const SwapChain& swapChain, 
			const VkSampleCountFlagBits msaaSamples
		);
		~DepthBuffer();

		void Resize(const uint32_t width, const uint32_t height);

		VkFormat FindDepthFormat();
		VkImageView GetDepthBufferImageView() { return m_DepthBufferImage->ImageView; }
	private:
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, 
			VkFormatFeatureFlags features);
		bool HasStencilComponent(VkFormat format);
	private:
		std::unique_ptr<class Image> m_DepthBufferImage;

		VkPhysicalDevice* p_PhysicalDevice = nullptr;

		static const uint32_t MIPLEVELS = 1;
	};
}
