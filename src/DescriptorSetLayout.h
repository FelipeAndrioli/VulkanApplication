#pragma once

#include <stdexcept>
#include <vector>

#include "Vulkan.h"
#include "DescriptorBinding.h"

namespace Engine {
	class DescriptorSetLayout {
	public:
		DescriptorSetLayout(std::vector<DescriptorBinding> descriptorBindings, VkDevice& logicalDevice);
		~DescriptorSetLayout();

		inline VkDescriptorSetLayout& GetHandle() { return m_DescriptorSetLayout; };
	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;

		VkDevice& p_LogicalDevice;
	};
}
