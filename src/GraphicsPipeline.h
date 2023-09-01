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

#include "../Assets/Vertex.h"

namespace Engine {
	class LogicalDevice;
	class SwapChain;

	class GraphicsPipeline {
	public:
		GraphicsPipeline(const char* vertexShaderPath, const char* fragShaderPath, LogicalDevice* logicalDevice, SwapChain* swapChain,
			VkVertexInputBindingDescription _bindingDescription, std::array<VkVertexInputAttributeDescription, 2> _attributeDescriptions,
			VkPrimitiveTopology topology, Buffer* uniformBuffers);
		~GraphicsPipeline();

		inline VkPipeline& GetHandle() { return m_GraphicsPipeline; };
		inline RenderPass& GetRenderPass() { return *m_RenderPass; };
		inline PipelineLayout& GetPipelineLayout() { return *m_GraphicsPipelineLayout; };
		inline DescriptorSetLayout& GetDescriptorSetLayout() { return *m_DescriptorSetLayout; };
		inline DescriptorSets& GetDescriptorSets() { return *m_DescriptorSets; };
	private:
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;

		LogicalDevice* p_LogicalDevice;
		SwapChain* p_SwapChain;
		Buffer* p_UniformBuffers;

		std::unique_ptr<class DescriptorSetLayout> m_DescriptorSetLayout;
		std::unique_ptr<class DescriptorSets> m_DescriptorSets;
		std::unique_ptr<class DescriptorPool> m_DescriptorPool;
		std::unique_ptr<class PipelineLayout> m_GraphicsPipelineLayout;
		std::unique_ptr<class RenderPass> m_RenderPass;
	};
}
