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
		vkFreeCommandBuffers(p_LogicalDevice, p_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
	}

	VkCommandBuffer& CommandBuffer::GetCommandBuffer(uint32_t index) {
		ValidateIndex(index);

		return m_CommandBuffers[index];
	}

	VkCommandBuffer& CommandBuffer::Begin(uint32_t index) {
		ValidateIndex(index);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(m_CommandBuffers[index], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin command buffer!");
		}

		return m_CommandBuffers[index];
	}

	void CommandBuffer::End(uint32_t index) {
		ValidateIndex(index);

		if (vkEndCommandBuffer(m_CommandBuffers[index]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to end command buffer!");
		}
	}

	VkCommandBuffer CommandBuffer::BeginSingleTimeCommandBuffer(VkDevice& logicalDevice, VkCommandPool& commandPool) {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = {};
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create one time command buffer!");
		}

		return commandBuffer;
	}

	void CommandBuffer::EndSingleTimeCommandBuffer(
		VkDevice& logicalDevice, 
		VkQueue& queue, 
		VkCommandBuffer& commandBuffer, 
		VkCommandPool& commandPool
	) {

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
	}

	void CommandBuffer::ValidateIndex(uint32_t index) {
		if (index > m_CommandBuffers.size()) {
			throw std::runtime_error("Index to access command buffer out of bounds!");
		}
	}
}