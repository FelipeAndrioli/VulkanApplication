#pragma once

#include <vector>
#include <array>
#include <string>
#include <stdexcept>

#include "Vulkan.h"
#include "../Assets/Shader.h"

namespace Engine {
	class PipelineLayout;

	class GraphicsPipeline {
	public:
		GraphicsPipeline(
			const Assets::VertexShader& vertexShader,
			const Assets::FragmentShader& fragmentShader,
			VulkanEngine& vulkanEngine,
			const VkRenderPass& renderPass,
			PipelineLayout& pipelineLayout);
		~GraphicsPipeline();

		inline VkPipeline& GetHandle() { return m_GraphicsPipeline; };
	private:
		VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
		PipelineLayout& m_GraphicsPipelineLayout;
		VulkanEngine& m_VulkanEngine;
	};

	class PipelineBuilder {
	public:
		PipelineBuilder() {};
		~PipelineBuilder() {};

		PipelineBuilder& AddVertexShader(Assets::VertexShader& vertexShader);
		PipelineBuilder& AddFragmentShader(Assets::FragmentShader& fragmentShader);
		PipelineBuilder& AddRenderPass(VkRenderPass& renderPass);
		PipelineBuilder& AddPipelineLayout(PipelineLayout& pipelineLayout);
		std::unique_ptr<class GraphicsPipeline> BuildGraphicsPipeline(VulkanEngine& vulkanEngine);

		Assets::VertexShader* m_VertexShader = nullptr;
		Assets::FragmentShader* m_FragmentShader = nullptr;
		VkRenderPass* m_RenderPass = nullptr;
		PipelineLayout* m_PipelineLayout = nullptr;
	};
}
