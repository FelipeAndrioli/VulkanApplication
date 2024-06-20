#include "Buffer.h"

#include "CommandPool.h"

namespace Engine {
	Buffer::Buffer(VulkanEngine& vulkanEngine, const size_t bufferSize, 
		const VkBufferUsageFlags usage) : m_VulkanEngine(&vulkanEngine), BufferSize(bufferSize) {

		BufferMemory.reset(new class DeviceMemory(
			vulkanEngine.GetLogicalDevice().GetHandle(), 
			vulkanEngine.GetPhysicalDevice().GetHandle()
		));

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize == 0 ? 256 : bufferSize;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(vulkanEngine.GetLogicalDevice().GetHandle(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create buffer!");
		}
	}

	Buffer::~Buffer() {
		vkDestroyBuffer(m_VulkanEngine->GetLogicalDevice().GetHandle(), m_Buffer, nullptr);
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

		vkCmdCopyBuffer(commandBuffer, srcBuffer, m_Buffer, 1, &copyRegion);

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

	void Buffer::Update(VkDeviceSize offset, void* data, size_t dataSize) {
		BufferMemory->MapMemory(offset, static_cast<VkDeviceSize>(dataSize));
		memcpy(BufferMemory->MemoryMapped, data, dataSize);
		BufferMemory->UnmapMemory();
	}

	void* Buffer::GetMappedMemory() {
		return BufferMemory->MemoryMapped;
	}
}