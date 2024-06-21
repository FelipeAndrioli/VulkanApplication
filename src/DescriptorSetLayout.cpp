#include "DescriptorSetLayout.h"

#include "Buffer.h"
#include "../Assets/Texture.h"

namespace Engine {
	DescriptorSetLayout::DescriptorSetLayout(std::vector<DescriptorBinding>& descriptorBindings, VkDevice& logicalDevice) 
	: p_LogicalDevice(logicalDevice), m_DescriptorBindings(descriptorBindings) {

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

	DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::NewBinding(uint32_t binding) {
		m_DescriptorBinding.binding = binding;
		return *this;
	}

	DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::SetDescriptorCount(uint32_t descriptorCount) {
		m_DescriptorBinding.descriptorCount = descriptorCount;
		return *this;
	}

	DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::SetType(VkDescriptorType type) {
		m_DescriptorBinding.type = type;
		return *this;
	}

	DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::SetStage(VkShaderStageFlags stage) {
		m_DescriptorBinding.stage = stage;
		return *this;
	}

	DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::Add() {
		m_DescriptorBindings.push_back(m_DescriptorBinding);
		m_DescriptorBinding.binding = 0;
		m_DescriptorBinding.descriptorCount = 0;
		m_DescriptorBinding.type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		
		return *this;
	}

	std::unique_ptr<class DescriptorSetLayout> DescriptorSetLayoutBuilder::Build(VkDevice& logicalDevice) {
		std::unique_ptr<class DescriptorSetLayout> newLayout = std::make_unique<class DescriptorSetLayout>(m_DescriptorBindings, logicalDevice);
		m_DescriptorBindings.clear();

		return newLayout;
	}
}