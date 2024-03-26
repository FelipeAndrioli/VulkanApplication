#include "Buffer.h"

#include "CommandPool.h"

namespace Engine {
	Buffer::Buffer(const int numBuffers, VulkanEngine& vulkanEngine, const size_t bufferSize, 
		const VkBufferUsageFlags usage) : m_VulkanEngine(&vulkanEngine), m_NumBuffers(numBuffers), BufferSize(bufferSize) {

		m_Buffer.resize(m_NumBuffers);
		BufferMemory.reset(new class DeviceMemory(
			vulkanEngine.GetLogicalDevice().GetHandle(), 
			vulkanEngine.GetPhysicalDevice().GetHandle(), 
			m_NumBuffers
		));

		for (size_t i = 0; i < m_NumBuffers; i++) {
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize == 0 ? 256 : bufferSize;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(vulkanEngine.GetLogicalDevice().GetHandle(), &bufferInfo, nullptr, &m_Buffer[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create buffer!");
			}
		}
	}

	Buffer::~Buffer() {
		if (!m_Buffer.empty()) {
			for (size_t i = 0; i < m_Buffer.size(); i++) {
				vkDestroyBuffer(m_VulkanEngine->GetLogicalDevice().GetHandle(), m_Buffer[i], nullptr);
			}
		}
		
		BufferMemory.reset();
	}

	void Buffer::AllocateMemory(VkMemoryPropertyFlags properties) {
		BufferMemory->AllocateMemory(m_Buffer, properties);
	}

	void Buffer::CopyFrom(VkBuffer srcBuffer, VkDeviceSize bufferSize, size_t srcOffset, size_t dstOffset) {

		VkCommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(
			m_VulkanEngine->GetLogicalDevice().GetHandle(), 
			m_VulkanEngine->GetCommandPool().GetHandle()
		);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = bufferSize;

		for (size_t i = 0; i < m_Buffer.size(); i++) {
			vkCmdCopyBuffer(commandBuffer, srcBuffer, m_Buffer[i], 1, &copyRegion);
		}

		CommandBuffer::EndSingleTimeCommandBuffer(
			m_VulkanEngine->GetLogicalDevice().GetHandle(),
			m_VulkanEngine->GetLogicalDevice().GetGraphicsQueue(),
			commandBuffer, 
			m_VulkanEngine->GetCommandPool().GetHandle()
		);
	}

	void Buffer::CopyToImage(
		VulkanEngine& vulkanEngine,
		VkQueue& queue, 
		VkImage& image, 
		const uint32_t imageWidth,
		const uint32_t imageHeight,
		VkImageLayout imageLayout,
		VkBuffer& buffer
	) {
		VkCommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(
			vulkanEngine.GetLogicalDevice().GetHandle(), 
			vulkanEngine.GetCommandPool().GetHandle()
		);

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
			commandBuffer, 
			buffer, 
			image, 
			imageLayout, 
			1, 
			&region
		);

		CommandBuffer::EndSingleTimeCommandBuffer(
			vulkanEngine.GetLogicalDevice().GetHandle(),
			vulkanEngine.GetLogicalDevice().GetGraphicsQueue(),
			commandBuffer, 
			vulkanEngine.GetCommandPool().GetHandle()
		);
	}

	void Buffer::NewChunk(BufferChunk newChunk) {
		Chunks.push_back(newChunk);
	}

	void Buffer::Update(uint32_t bufferIndex, VkDeviceSize offset, void* data, size_t dataSize) {
		BufferMemory->MapMemory(bufferIndex, offset);
		memcpy(BufferMemory->MemoryMapped[bufferIndex], data, dataSize);
		BufferMemory->UnmapMemory(bufferIndex);
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