#include "DescriptorSetLayout.h"

namespace Engine {
	DescriptorSetLayout::DescriptorSetLayout(VkDevice& logicalDevice) : p_LogicalDevice(logicalDevice) {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(p_LogicalDevice, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}
	}

	DescriptorSetLayout::~DescriptorSetLayout() {
		vkDestroyDescriptorSetLayout(p_LogicalDevice, m_DescriptorSetLayout, nullptr);
	}
}