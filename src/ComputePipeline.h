#pragma once

#include "Vulkan.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "ShaderModule.h"
#include "DescriptorSetLayout.h"
#include "PipelineLayout.h"

namespace Engine {
	class ComputePipeline {
	public:
		ComputePipeline(const char* computeShaderPath, LogicalDevice* logicalDevice, SwapChain* swapChain);
		~ComputePipeline();

		inline VkPipeline& GetHandle() { return m_ComputePipeline; };
		inline PipelineLayout& GetPipelineLayout() { return  *m_PipelineLayout; };
		inline DescriptorSetLayout& GetDescriptorSetLayout() { return *m_DescriptorSetLayout; };
	private:
		VkPipeline m_ComputePipeline;

		std::unique_ptr<class DescriptorSetLayout> m_DescriptorSetLayout;
		std::unique_ptr<class PipelineLayout> m_PipelineLayout;

		LogicalDevice* p_LogicalDevice;
		SwapChain* p_SwapChain;
	};
}
