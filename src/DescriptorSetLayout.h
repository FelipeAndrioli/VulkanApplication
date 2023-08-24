#pragma once

#include <stdexcept>

#include "Vulkan.h"

namespace Engine {
	class DescriptorSetLayout {
	public:
		DescriptorSetLayout(VkDevice& logicalDevice);
		~DescriptorSetLayout();

		inline VkDescriptorSetLayout& GetHandle() { return m_DescriptorSetLayout; };
	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;

		VkDevice& p_LogicalDevice;
	};
}
