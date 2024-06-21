#pragma once

#include <stdexcept>
#include <vector>
#include <memory>

#include "Vulkan.h"
#include "DescriptorBinding.h"

namespace Assets {
	struct Texture;
}

namespace Engine {
	class Buffer;
	class DescriptorSetLayout {
	public:
		DescriptorSetLayout(std::vector<DescriptorBinding>& descriptorBindings, VkDevice& logicalDevice);
		~DescriptorSetLayout();

		inline VkDescriptorSetLayout& GetHandle() { return m_DescriptorSetLayout; };
		std::vector<DescriptorBinding>& GetDescriptorBindings() { return m_DescriptorBindings; }
	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;
		std::vector<DescriptorBinding> m_DescriptorBindings;
		VkDevice& p_LogicalDevice;
	};

	class DescriptorSetLayoutBuilder {
	public:
		DescriptorSetLayoutBuilder() {};
		~DescriptorSetLayoutBuilder() {};

		DescriptorSetLayoutBuilder& NewBinding(uint32_t binding);
		DescriptorSetLayoutBuilder& SetDescriptorCount(uint32_t descriptorCount);
		DescriptorSetLayoutBuilder& SetType(VkDescriptorType type);
		DescriptorSetLayoutBuilder& SetStage(VkShaderStageFlags stage);
		DescriptorSetLayoutBuilder& Add();
		std::unique_ptr<class DescriptorSetLayout> Build(VkDevice& logicalDevice);

	private:
		std::vector<DescriptorBinding> m_DescriptorBindings;
		DescriptorBinding m_DescriptorBinding;
	};
}
