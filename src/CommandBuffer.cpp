#include "CommandBuffer.h"

namespace Engine {
	CommandBuffer::CommandBuffer(uint32_t size, VkCommandPool& commandPool, VkDevice& logicalDevice) 
		: p_CommandPool(commandPool), p_LogicalDevice(logicalDevice) {

		m_CommandBuffers.resize(size);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = p_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = size;

		if (vkAllocateCommandBuffers(p_LogicalDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate compute command buffers!");
		}
	}

	CommandBuffer::~CommandBuffer() {
		vkFreeCommandBuffers(p_LogicalDevice, p_CommandPool, m_CommandBuffers.size(), m_CommandBuffers.data());
	}

	VkCommandBuffer& CommandBuffer::GetCommandBuffer(uint32_t index) {
		if (index > m_CommandBuffers.size()) {
			throw std::runtime_error("Index to access command buffer out of bounds!");
		}

		return m_CommandBuffers[index];
	}
}