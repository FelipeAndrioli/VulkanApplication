#include "Pipeline.h"

#include "LogicalDevice.h"
#include "SwapChain.h"
#include "ShaderModule.h"
#include "DescriptorBinding.h"
#include "PipelineLayout.h"

namespace Engine {
	GraphicsPipeline::GraphicsPipeline(
		const Assets::VertexShader& vertexShader,
		const Assets::FragmentShader& fragmentShader, 
		VulkanEngine& vulkanEngine,
		const VkRenderPass& renderPass,
		PipelineLayout& pipelineLayout)
		: m_VulkanEngine(vulkanEngine), m_GraphicsPipelineLayout(pipelineLayout) {

		auto bindingDescription = vertexShader.BindingDescription;
		auto attributeDescriptions = vertexShader.AttributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = static_cast<VkPrimitiveTopology>(fragmentShader.TopologyMode);
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkExtent2D swapChainExtent = vulkanEngine.GetSwapChain().GetSwapChainExtent();
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = static_cast<VkPolygonMode>(fragmentShader.PolygonMode);
		rasterizer.lineWidth = fragmentShader.LineWidth;
		rasterizer.cullMode = fragmentShader.CullingMode;
		rasterizer.frontFace = static_cast<VkFrontFace>(fragmentShader.FrontFaceMode);
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = vulkanEngine.GetPhysicalDevice().GetMsaaSamples();
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		std::vector<BufferDescriptor> bufferDescriptor = {};

		ShaderModule vertShaderModule(vertexShader.Path.c_str(), vulkanEngine.GetLogicalDevice(), VK_SHADER_STAGE_VERTEX_BIT);
		ShaderModule fragShaderModule(fragmentShader.Path.c_str(), vulkanEngine.GetLogicalDevice(), VK_SHADER_STAGE_FRAGMENT_BIT);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderModule.GetShaderStageInfo(), fragShaderModule.GetShaderStageInfo() };

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCreateInfo.minDepthBounds = 0.0f;
		depthStencilCreateInfo.maxDepthBounds = 1.0f;
		depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
		depthStencilCreateInfo.front = {};
		depthStencilCreateInfo.back = {};

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencilCreateInfo;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_GraphicsPipelineLayout.GetHandle();
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		//pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(vulkanEngine.GetLogicalDevice().GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo,
			nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline!");
		}
	}

	GraphicsPipeline::~GraphicsPipeline() {
		vkDestroyPipeline(m_VulkanEngine.GetLogicalDevice().GetHandle(), m_GraphicsPipeline, nullptr);
	}

	PipelineBuilder& PipelineBuilder::AddVertexShader(Assets::VertexShader& vertexShader) {
		m_VertexShader = &vertexShader;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::AddFragmentShader(Assets::FragmentShader& fragmentShader) {
		m_FragmentShader = &fragmentShader;
		return *this;
	}

	PipelineBuilder& PipelineBuilder::AddRenderPass(VkRenderPass& renderPass) {
		m_RenderPass = &renderPass;
		return *this;
	}
	
	PipelineBuilder& PipelineBuilder::AddPipelineLayout(PipelineLayout& pipelineLayout) {
		m_PipelineLayout = &pipelineLayout;
		return *this;
	}

	std::unique_ptr<class GraphicsPipeline> PipelineBuilder::BuildGraphicsPipeline(VulkanEngine& vulkanEngine) {
		std::unique_ptr<class GraphicsPipeline> graphicsPipeline = std::make_unique<class GraphicsPipeline>(
			*m_VertexShader,
			*m_FragmentShader,
			vulkanEngine,
			*m_RenderPass,
			*m_PipelineLayout
		);

		m_VertexShader = nullptr;
		m_FragmentShader = nullptr;
		m_RenderPass = nullptr;
		m_PipelineLayout = nullptr;

		return graphicsPipeline;
	}
}
