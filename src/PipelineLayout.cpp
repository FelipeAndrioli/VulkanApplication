#include "PipelineLayout.h"

#include "../Assets/Texture.h"

namespace Engine {
	PipelineLayout::PipelineLayout(
		VkDevice& logicalDevice, 
		std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, 
		std::vector<VkPushConstantRange>& pushConstantRanges
	) : p_LogicalDevice(logicalDevice) {

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

		if (vkCreatePipelineLayout(p_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}
	}

	PipelineLayout::~PipelineLayout() {
		vkDestroyPipelineLayout(p_LogicalDevice, m_PipelineLayout, nullptr);
	}

	PipelineLayoutBuilder& PipelineLayoutBuilder::AddDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout) {
		m_DescriptorSetLayouts.push_back(descriptorSetLayout);
		return *this;
	}

	PipelineLayoutBuilder& PipelineLayoutBuilder::AddPushConstant(VkPushConstantRange& pushConstant) {
		m_PushConstantRanges.push_back(pushConstant);
		return *this;
	}
	
	std::unique_ptr<class PipelineLayout> PipelineLayoutBuilder::BuildPipelineLayout(VkDevice& logicalDevice) {
		std::unique_ptr<class PipelineLayout> pipelineLayout = std::make_unique<class PipelineLayout>(
			logicalDevice,
			m_DescriptorSetLayouts,
			m_PushConstantRanges
		);

		m_DescriptorSetLayouts.clear();
		m_PushConstantRanges.clear();

		return pipelineLayout;
	}
}