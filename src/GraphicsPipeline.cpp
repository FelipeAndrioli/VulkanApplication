#include "GraphicsPipelineLayout.h"
#include "GraphicsPipeline.h"
#include "LogicalDevice.h"
#include "SwapChain.h"

namespace Engine {
	GraphicsPipeline::GraphicsPipeline(GraphicsPipelineLayout* graphicsPipelineLayout, LogicalDevice* logicalDevice, SwapChain* swapChain)
		: p_LogicalDevice(logicalDevice), p_SwapChain(swapChain) {

		auto bindingDescription = graphicsPipelineLayout->bindingDescription;
		auto attributeDescriptions = graphicsPipelineLayout->attributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = static_cast<VkPrimitiveTopology>(graphicsPipelineLayout->topology);
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)p_SwapChain->GetSwapChainExtent().width;
		viewport.height = (float)p_SwapChain->GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = p_SwapChain->GetSwapChainExtent();

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
		rasterizer.polygonMode = static_cast<VkPolygonMode>(graphicsPipelineLayout->polygonMode);
		rasterizer.lineWidth = graphicsPipelineLayout->lineWidth;
		rasterizer.cullMode = graphicsPipelineLayout->cullMode;
		rasterizer.frontFace = static_cast<VkFrontFace>(graphicsPipelineLayout->frontFace);
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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

		std::vector<DescriptorBinding> descriptorBindings = {
			{ 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }
		};

		m_DescriptorSetLayout.reset(new class DescriptorSetLayout(descriptorBindings, p_LogicalDevice->GetHandle()));

		std::vector<PoolDescriptorBinding> poolDescriptorBindings = {};

		for (size_t i = 0; i < graphicsPipelineLayout->maxDescriptorSets; i++) {
			poolDescriptorBindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT });
			poolDescriptorBindings.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT * 2 });

			// We need to double the number of VK_DESCRIPTOR_TYPE_STORAGE_BUFFER types requested from the pool
			// because our sets reference the SSBOs of the last and current frame (for now).
		}

		m_DescriptorPool.reset(new class DescriptorPool(p_LogicalDevice->GetHandle(), poolDescriptorBindings, 
			static_cast<uint32_t>(graphicsPipelineLayout->maxDescriptorSets * MAX_FRAMES_IN_FLIGHT)));

		std::vector<BufferDescriptor> bufferDescriptor = {};

		m_GraphicsPipelineLayout.reset(new class PipelineLayout(p_LogicalDevice->GetHandle(), m_DescriptorSetLayout.get()));
		m_RenderPass.reset(new class RenderPass(p_SwapChain, p_LogicalDevice->GetHandle()));

		ShaderModule vertShaderModule(graphicsPipelineLayout->vertexShaderPath, p_LogicalDevice, VK_SHADER_STAGE_VERTEX_BIT);
		ShaderModule fragShaderModule(graphicsPipelineLayout->fragmentShaderPath, p_LogicalDevice, VK_SHADER_STAGE_FRAGMENT_BIT);

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderModule.GetShaderStageInfo(), fragShaderModule.GetShaderStageInfo() };

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_GraphicsPipelineLayout->GetHandle();
		pipelineInfo.renderPass = m_RenderPass->GetHandle();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		//pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(p_LogicalDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo,
			nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline!");
		}
	}

	GraphicsPipeline::~GraphicsPipeline() {
		vkDestroyPipeline(p_LogicalDevice->GetHandle(), m_GraphicsPipeline, nullptr);

		m_RenderPass.reset();
		m_GraphicsPipelineLayout.reset();
		m_DescriptorSetLayout.reset();
	}
}
