#pragma once

#include <vector>
#include <stdexcept>

#include "Vulkan.h"
#include "SwapChain.h"
#include "Image.h"

namespace Engine {
	class DepthBuffer {
	public:
		DepthBuffer(VkPhysicalDevice* physicalDevice, VkDevice* logicalDevice, SwapChain* swapChain);
		~DepthBuffer();

		void Resize(SwapChain* swapChain);

		VkFormat FindDepthFormat();
		std::vector<VkImageView> GetDepthBufferImageView() { return m_DepthBufferImage->ImageView; }
	private:
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, 
			VkFormatFeatureFlags features);
		bool HasStencilComponent(VkFormat format);
	private:
		std::unique_ptr<class Image> m_DepthBufferImage;

		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
