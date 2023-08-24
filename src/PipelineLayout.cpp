#include "PipelineLayout.h"

namespace Engine {
	PipelineLayout::PipelineLayout(VkDevice& logicalDevice, DescriptorSetLayout* descriptorSetLayout) : p_LogicalDevice(logicalDevice) {
		VkDescriptorSetLayout descriptorSetLayouts[] = { descriptorSetLayout->GetHandle() };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
		//pipelineLayoutInfo.setLayoutCount = 0;
		//pipelineLayoutInfo.pSetLayouts = nullptr;

		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(p_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	PipelineLayout::~PipelineLayout() {
		vkDestroyPipelineLayout(p_LogicalDevice, m_PipelineLayout, nullptr);
	}
}