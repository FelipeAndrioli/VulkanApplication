#pragma once

#include <stdexcept>

#include "Vulkan.h"
#include "Common.h"

namespace Engine {
	class CommandPool {
	public:
		CommandPool(VkDevice& logicalDevice, QueueFamilyIndices familyIndices);
		~CommandPool();

		inline VkCommandPool& GetHandle() { return m_CommandPool; };
	private:
		VkCommandPool m_CommandPool;

		VkDevice& p_LogicalDevice;
	};
}
