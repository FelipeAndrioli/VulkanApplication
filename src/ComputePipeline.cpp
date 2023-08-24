#include "ComputePipeline.h"

namespace Engine {
	ComputePipeline::ComputePipeline(const char* computeShaderPath, LogicalDevice* logicalDevice, SwapChain* swapChain) 
		: p_LogicalDevice(logicalDevice), p_SwapChain(swapChain) {

		ShaderModule computeShader(computeShaderPath, p_LogicalDevice, VK_SHADER_STAGE_COMPUTE_BIT);

		VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = computeShader.GetHandle();
		computeShaderStageInfo.pName = "main";

		std::vector<DescriptorBinding> descriptorBindings = {
			{ 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
			{ 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
			{ 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
		};

		m_DescriptorSetLayout.reset(new class DescriptorSetLayout(descriptorBindings, p_LogicalDevice->GetHandle()));
		m_PipelineLayout.reset(new class PipelineLayout(p_LogicalDevice->GetHandle(), m_DescriptorSetLayout.get()));

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = m_PipelineLayout->GetHandle();
		pipelineInfo.stage = computeShaderStageInfo;

		if (vkCreateComputePipelines(p_LogicalDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_ComputePipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Compute pipeline!");
		}
	}
	
	ComputePipeline::~ComputePipeline() {
		vkDestroyPipeline(p_LogicalDevice->GetHandle(), m_ComputePipeline, nullptr);

		m_DescriptorSetLayout.reset();
		m_PipelineLayout.reset();
	}
}