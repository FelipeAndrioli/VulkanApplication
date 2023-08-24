#include "DescriptorSetLayout.h"

namespace Engine {
	DescriptorSetLayout::DescriptorSetLayout(std::vector<DescriptorBinding> descriptorBindings, VkDevice& logicalDevice) 
	: p_LogicalDevice(logicalDevice){

		std::vector<VkDescriptorSetLayoutBinding> bindings = {};

		for (size_t i = 0; i < descriptorBindings.size(); i++) {
			VkDescriptorSetLayoutBinding newBinding = {};
			newBinding.binding = descriptorBindings[i].binding;
			newBinding.descriptorCount = descriptorBindings[i].descriptorCount;
			newBinding.descriptorType = descriptorBindings[i].type;
			newBinding.pImmutableSamplers = nullptr;
			newBinding.stageFlags = descriptorBindings[i].stage;

			bindings.push_back(newBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
	
		if (vkCreateDescriptorSetLayout(p_LogicalDevice, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	DescriptorSetLayout::~DescriptorSetLayout() {
		vkDestroyDescriptorSetLayout(p_LogicalDevice, m_DescriptorSetLayout, nullptr);
	}
}