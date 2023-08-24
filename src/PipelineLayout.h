#pragma once

#include <stdexcept>

#include "Vulkan.h"
#include "DescriptorSetLayout.h"

namespace Engine {
	class PipelineLayout {
	public:
		PipelineLayout(VkDevice& logicalDevice, DescriptorSetLayout* descriptorSetLayout);
		~PipelineLayout();

		inline VkPipelineLayout GetHandle() { return m_PipelineLayout; };
	private:
		VkPipelineLayout m_PipelineLayout;

		VkDevice& p_LogicalDevice;
	};
}
