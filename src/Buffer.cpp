#include "Buffer.h"

namespace Engine {
	Buffer::Buffer(const int bufferSize, LogicalDevice* logicalDevice, PhysicalDevice* physicalDevice, const size_t size, 
		const VkBufferUsageFlags usage) : p_LogicalDevice(logicalDevice), p_PhysicalDevice(physicalDevice) {

		m_BufferSize = bufferSize;
		m_Buffer.resize(m_BufferSize);
		m_BufferMemory.resize(m_BufferSize);
		m_BufferMemoryMapped.resize(m_BufferSize);

		for (size_t i = 0; i < m_BufferSize; i++) {
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(p_LogicalDevice->GetHandle(), &bufferInfo, nullptr, &m_Buffer[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create buffer!");
			}
		}
	}

	Buffer::~Buffer() {
		if (!m_Buffer.empty()) {
			for (size_t i = 0; i < m_Buffer.size(); i++) {
				vkDestroyBuffer(p_LogicalDevice->GetHandle(), m_Buffer[i], nullptr);
				vkFreeMemory(p_LogicalDevice->GetHandle(), m_BufferMemory[i], nullptr);
			}
		}
	}

	void Buffer::AllocateMemory(VkMemoryPropertyFlags properties) {
		for (size_t i = 0; i < m_Buffer.size(); i++) {
			VkMemoryRequirements memRequirements = GetMemoryRequirements(m_Buffer[i]);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
		
			if (vkAllocateMemory(p_LogicalDevice->GetHandle(), &allocInfo, nullptr, &m_BufferMemory[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to allocate buffer memory!");
			}

			vkBindBufferMemory(p_LogicalDevice->GetHandle(), m_Buffer[i], m_BufferMemory[i], 0);
		}
	}

	void Buffer::MapMemory() {
		for (size_t i = 0; i < m_Buffer.size(); i++) {
			vkMapMemory(p_LogicalDevice->GetHandle(), m_BufferMemory[i], 0, m_Buffer.size(), 0, &m_BufferMemoryMapped[i]);
		}
	}

	void Buffer::MapMemory(void* data) {
		for (size_t i = 0; i < m_Buffer.size(); i++) {
			vkMapMemory(p_LogicalDevice->GetHandle(), m_BufferMemory[i], 0, m_Buffer.size(), 0, &data);
		}
	}

	void Buffer::UnmapMemory() {
		for (size_t i = 0; i < m_Buffer.size(); i++) {
			vkUnmapMemory(p_LogicalDevice->GetHandle(), m_BufferMemory[i]);
		}
	}

	void Buffer::CopyFrom(VkBuffer srcBuffer, VkDeviceSize bufferSize, VkCommandPool& commandPool) {

		VkCommandBuffer commandBuffer = beginSingleTimeCommands(p_LogicalDevice->GetHandle(), commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = bufferSize;

		for (size_t i = 0; i < m_Buffer.size(); i++) {
			vkCmdCopyBuffer(commandBuffer, srcBuffer, m_Buffer[i], 1, &copyRegion);
		}

		endSingleTimeCommands(p_LogicalDevice->GetHandle(), p_LogicalDevice->GetGraphicsQueue(), 
			commandBuffer, commandPool);
	}

	VkBuffer& Buffer::GetBuffer(uint32_t index) {
		if (index > m_Buffer.size() || index < 0) {
			throw std::runtime_error("Index to retrieve buffer out of bounds!");
		}

		return m_Buffer[index];
	}

	VkDeviceMemory& Buffer::GetBufferMemory(uint32_t index) {
		if (index > m_Buffer.size() || index < 0) {
			throw std::runtime_error("Index to retrieve buffer memory out of bounds!");
		}

		return m_BufferMemory[index];
	}

	void* Buffer::GetBufferMemoryMapped(uint32_t index) {
		if (index > m_Buffer.size() || index < 0) {
			throw std::runtime_error("Index to retrieve buffer memory mapped out of bounds!");
		}

		return m_BufferMemoryMapped[index];
	}

	uint32_t Buffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(p_PhysicalDevice->GetHandle(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	VkMemoryRequirements Buffer::GetMemoryRequirements(VkBuffer& buffer) {
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(p_LogicalDevice->GetHandle(), buffer, &memRequirements);

		return memRequirements;
	}
}