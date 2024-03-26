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
}
