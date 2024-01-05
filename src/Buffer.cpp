#include "Buffer.h"

namespace Engine {
	Buffer::Buffer(const int bufferQuantity, LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, const size_t bufferSize, 
		const VkBufferUsageFlags usage) : p_LogicalDevice(&logicalDevice), m_BufferSize(bufferQuantity) {

		m_Buffer.resize(m_BufferSize);
		BufferMemory.reset(new class DeviceMemory(&p_LogicalDevice->GetHandle(), &physicalDevice.GetHandle(), m_BufferSize));

		for (size_t i = 0; i < m_BufferSize; i++) {
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
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

		VkCommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(p_LogicalDevice->GetHandle(), commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = bufferSize;

		for (size_t i = 0; i < m_Buffer.size(); i++) {
			vkCmdCopyBuffer(commandBuffer, srcBuffer, m_Buffer[i], 1, &copyRegion);
		}

		CommandBuffer::EndSingleTimeCommandBuffer(p_LogicalDevice->GetHandle(), p_LogicalDevice->GetGraphicsQueue(), 
			commandBuffer, commandPool);
	}

	void Buffer::CopyToImage(
		VkDevice& logicalDevice, 
		VkCommandPool& commandPool, 
		VkQueue& queue, 
		VkImage& image, 
		const uint32_t imageWidth,
		const uint32_t imageHeight,
		VkImageLayout imageLayout,
		VkBuffer& buffer
	) {
		VkCommandBuffer sCommandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(logicalDevice, commandPool);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { imageWidth, imageHeight, 1 }; 

		vkCmdCopyBufferToImage(
			sCommandBuffer, 
			buffer, 
			image, 
			imageLayout, 
			1, 
			&region
		);

		CommandBuffer::EndSingleTimeCommandBuffer(logicalDevice, queue, sCommandBuffer, commandPool);
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