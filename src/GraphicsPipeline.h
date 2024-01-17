#pragma once

#include <vector>
#include <array>
#include <string>
#include <stdexcept>

#include "Vulkan.h"
#include "RenderPass.h"
#include "PipelineLayout.h"
#include "DescriptorSetLayout.h"
#include "ShaderModule.h"
#include "DescriptorBinding.h"
#include "DescriptorSets.h"
#include "DescriptorPool.h"
#include "Buffer.h"
#include "DepthBuffer.h"

#include "../Assets/Shader.h"

namespace Engine {
	class LogicalDevice;
	class SwapChain;

	class GraphicsPipeline {
	public:
		GraphicsPipeline(
			const Assets::VertexShader& vertexShader,
			const Assets::FragmentShader& fragmentShader,
			LogicalDevice& logicalDevice,
			const SwapChain& swapChain,
			const DepthBuffer& depthBuffer,
			const VkRenderPass& renderPass,
			std::vector<DescriptorSetLayout*> descriptorSetLayouts);
		~GraphicsPipeline();

		inline VkPipeline& GetHandle() { return m_GraphicsPipeline; };
		inline PipelineLayout& GetPipelineLayout() { return *m_GraphicsPipelineLayout; };
	private:
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
		LogicalDevice* p_LogicalDevice = nullptr;

		std::unique_ptr<class PipelineLayout> m_GraphicsPipelineLayout;
	};
}
