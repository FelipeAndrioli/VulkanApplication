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

#include "../Assets/Vertex.h"

namespace Engine {
	class LogicalDevice;
	class SwapChain;

	class GraphicsPipeline {
	public:
		GraphicsPipeline(const char* vertexShaderPath, const char* fragShaderPath, LogicalDevice* logicalDevice, SwapChain* swapChain,
			VkVertexInputBindingDescription _bindingDescription, std::array<VkVertexInputAttributeDescription, 2> _attributeDescriptions, VkPrimitiveTopology topology);
		~GraphicsPipeline();

		inline VkPipeline& GetHandle() { return m_GraphicsPipeline; };
		inline RenderPass& GetRenderPass() { return *m_RenderPass; };
		inline PipelineLayout& GetPipelineLayout() { return *m_GraphicsPipelineLayout; };
		inline DescriptorSetLayout& GetDescriptorSetLayout() { return *m_DescriptorSetLayout; };
	private:
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;

		LogicalDevice* p_LogicalDevice;
		SwapChain* p_SwapChain;

		std::unique_ptr<class DescriptorSetLayout> m_DescriptorSetLayout;
		std::unique_ptr<class PipelineLayout> m_GraphicsPipelineLayout;
		std::unique_ptr<class RenderPass> m_RenderPass;
	};
}
