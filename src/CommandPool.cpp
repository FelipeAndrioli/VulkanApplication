#include "CommandPool.h"

namespace Engine {
	CommandPool::CommandPool(VkDevice& logicalDevice, QueueFamilyIndices familyIndices) : p_LogicalDevice(logicalDevice) {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(p_LogicalDevice, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Command Pool!");
		}
	}
	
	CommandPool::~CommandPool() {
		vkDestroyCommandPool(p_LogicalDevice, m_CommandPool, nullptr);
	}
}