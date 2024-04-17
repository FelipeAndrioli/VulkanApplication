#pragma once

#include <stdexcept>
#include <memory>

#include "Vulkan.h"
#include "DescriptorSetLayout.h"

namespace Engine {
	class PipelineLayout {
	public:
		PipelineLayout(
			VkDevice& logicalDevice, 
			std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, 
			std::vector<VkPushConstantRange>& pushConstantRanges
		);
		~PipelineLayout();

		inline VkPipelineLayout GetHandle() { return m_PipelineLayout; };
	private:
		VkPipelineLayout m_PipelineLayout;

		VkDevice& p_LogicalDevice;
	};

	class PipelineLayoutBuilder {
	public:
		PipelineLayoutBuilder() {};
		~PipelineLayoutBuilder() {};

		PipelineLayoutBuilder& AddDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout);
		PipelineLayoutBuilder& AddPushConstant(VkPushConstantRange& pushConstant);
		std::unique_ptr<class PipelineLayout> BuildPipelineLayout(VkDevice& logicalDevice);

	private:
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		std::vector<VkPushConstantRange> m_PushConstantRanges;
	};
}
