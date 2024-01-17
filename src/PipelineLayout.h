#pragma once

#include <stdexcept>
#include <memory>

#include "Vulkan.h"
#include "DescriptorSetLayout.h"

namespace Engine {
	class PipelineLayout {
	public:
		PipelineLayout(VkDevice& logicalDevice, std::vector<DescriptorSetLayout*> descriptorSetLayouts);
		~PipelineLayout();

		inline VkPipelineLayout GetHandle() { return m_PipelineLayout; };
	private:
		VkPipelineLayout m_PipelineLayout;

		VkDevice& p_LogicalDevice;
	};
}
