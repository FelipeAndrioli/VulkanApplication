#pragma once

#include "Vulkan.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "ShaderModule.h"
#include "DescriptorSetLayout.h"
#include "PipelineLayout.h"
#include "DescriptorPool.h"
#include "DescriptorSets.h"
#include "MaterialLayout.h"

namespace Engine {
	// TODO: For later
	/*
	class ComputePipeline {
	public:
		ComputePipeline(ComputePipelineLayout* computePipelineLayout, LogicalDevice* logicalDevice, 
			SwapChain* swapChain, Buffer* shaderStorageBuffers, Buffer* uniformBuffers);
		~ComputePipeline();

		inline VkPipeline& GetHandle() { return m_ComputePipeline; };
		inline PipelineLayout& GetPipelineLayout() { return  *m_PipelineLayout; };
		inline DescriptorSetLayout& GetDescriptorSetLayout() { return *m_DescriptorSetLayout; };
		inline DescriptorSets& GetDescriptorSets() { return *m_DescriptorSets; };
	private:
		VkPipeline m_ComputePipeline;

		std::unique_ptr<class DescriptorSetLayout> m_DescriptorSetLayout;
		std::unique_ptr<class PipelineLayout> m_PipelineLayout;
		std::unique_ptr<class DescriptorSets> m_DescriptorSets;
		std::unique_ptr<class DescriptorPool> m_DescriptorPool;

		LogicalDevice* p_LogicalDevice;
		SwapChain* p_SwapChain;
		Buffer* p_UniformBuffers;
		Buffer* p_ShaderStorageBuffers;
	};
	*/
}
