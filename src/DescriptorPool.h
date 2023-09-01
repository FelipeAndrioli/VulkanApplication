#pragma once

#include <array>
#include <vector>
#include <stdexcept>

#include "Vulkan.h"
#include "DescriptorBinding.h"
#include "DescriptorPool.h"

namespace Engine {
	class DescriptorPool {
	public:
		DescriptorPool(VkDevice& logicalDevice, std::vector<PoolDescriptorBinding> descriptorBindings, uint32_t maxSets);
		~DescriptorPool();

		inline VkDescriptorPool& GetHandle() { return m_DescriptorPool; };
	private:
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

		VkDevice& p_LogicalDevice;
	};
}
