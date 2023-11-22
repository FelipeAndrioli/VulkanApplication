#include "Buffer.h"

namespace Engine {
	Buffer::Buffer(const int bufferSize, LogicalDevice* logicalDevice, PhysicalDevice* physicalDevice, const size_t size, 
		const VkBufferUsageFlags usage) : p_LogicalDevice(logicalDevice), p_PhysicalDevice(physicalDevice), m_BufferSize(bufferSize) {

		m_Buffer.resize(m_BufferSize);
		BufferMemory.reset(new class DeviceMemory(&p_LogicalDevice->GetHandle(), &p_PhysicalDevice->GetHandle(), m_BufferSize));

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
			}
		}

		BufferMemory.reset();
	}


	void Buffer::AllocateMemory(VkMemoryPropertyFlags properties) {
		BufferMemory->AllocateMemory(m_Buffer, properties);
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

	void* Buffer::GetBufferMemoryMapped(uint32_t index) {
		if (index > m_Buffer.size() || index < 0) {
			throw std::runtime_error("Index to retrieve buffer memory mapped out of bounds!");
		}
		
		return BufferMemory->MemoryMapped[index];
	}
}