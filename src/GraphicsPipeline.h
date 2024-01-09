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
			const VkRenderPass& renderPass);
		~GraphicsPipeline();

		inline VkPipeline& GetHandle() { return m_GraphicsPipeline; };
		inline PipelineLayout& GetPipelineLayout() { return *m_GraphicsPipelineLayout; };
		inline DescriptorSetLayout& GetDescriptorSetLayout() { return *m_DescriptorSetLayout; };
		inline DescriptorPool& GetDescriptorPool() { return *m_DescriptorPool; };
	private:
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;

		LogicalDevice* p_LogicalDevice = nullptr;

		std::unique_ptr<class DescriptorSetLayout> m_DescriptorSetLayout;
		std::unique_ptr<class DescriptorPool> m_DescriptorPool;
		std::unique_ptr<class PipelineLayout> m_GraphicsPipelineLayout;
	};
}
