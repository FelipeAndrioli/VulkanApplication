#pragma once

#include <stdexcept>

#include "Vulkan.h"
#include "DepthBuffer.h"

namespace Engine {
	class SwapChain;

	class RenderPass {
	public:
		RenderPass(SwapChain* swapChain, VkDevice& logicalDevice, DepthBuffer* depthBuffer);
		~RenderPass();

		inline VkRenderPass& GetHandle() { return m_RenderPass; };
	private:
		VkRenderPass m_RenderPass;
		VkDevice& p_LogicalDevice;
	};
}
