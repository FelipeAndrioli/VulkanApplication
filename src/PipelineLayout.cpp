#include "PipelineLayout.h"

#include "../Assets/Texture.h"

namespace Engine {
	PipelineLayout::PipelineLayout(VkDevice& logicalDevice, std::vector<DescriptorSetLayout*> descriptorSetLayouts) 
		: p_LogicalDevice(logicalDevice) {
		std::vector<VkDescriptorSetLayout> innerDescriptorSetLayouts = {};

		for (auto& descriptorSetLayout : descriptorSetLayouts) {
			innerDescriptorSetLayouts.push_back(descriptorSetLayout->GetHandle());
		}

		VkPushConstantRange pushConstant = {};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(int);
		pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(innerDescriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = innerDescriptorSetLayouts.data();
		//pipelineLayoutInfo.setLayoutCount = 0;
		//pipelineLayoutInfo.pSetLayouts = nullptr;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

		if (vkCreatePipelineLayout(p_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	PipelineLayout::~PipelineLayout() {
		vkDestroyPipelineLayout(p_LogicalDevice, m_PipelineLayout, nullptr);
	}
}