#include "ComputePipeline.h"

namespace Engine {
	/*
	ComputePipeline::ComputePipeline(ComputePipelineLayout* computePipelineLayout, LogicalDevice* logicalDevice, 
		SwapChain* swapChain, Buffer* shaderStorageBuffers, Buffer* uniformBuffers) 
		: p_LogicalDevice(logicalDevice), p_SwapChain(swapChain), p_ShaderStorageBuffers(shaderStorageBuffers), 
		p_UniformBuffers(uniformBuffers) {

		ShaderModule computeShader(computePipelineLayout->computeShaderPath, p_LogicalDevice, VK_SHADER_STAGE_COMPUTE_BIT);

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

		std::vector<PoolDescriptorBinding> poolDescriptorBindings = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2 }
		};

		m_DescriptorPool.reset(new class DescriptorPool(p_LogicalDevice->GetHandle(), poolDescriptorBindings, MAX_FRAMES_IN_FLIGHT));

		m_DescriptorSets.reset(new class DescriptorSets( 
			sizeof(UniformBufferObject),
			p_LogicalDevice->GetHandle(),
			m_DescriptorPool->GetHandle(),
			m_DescriptorSetLayout->GetHandle(),
			p_UniformBuffers,
			p_ShaderStorageBuffers,
			computePipelineLayout->lastFrameAccess
		));

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
	*/
}