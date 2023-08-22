#pragma once

#include <stdexcept>

#include "Vulkan.h"

namespace Engine {
	class SwapChain;

	class RenderPass {
	public:
		RenderPass(SwapChain* swapChain, VkDevice& logicalDevice);
		~RenderPass();

		inline VkRenderPass& GetHandle() { return m_RenderPass; };
	private:
		VkRenderPass m_RenderPass;

		SwapChain* p_SwapChain;
		VkDevice& p_LogicalDevice;
	};
}
