#pragma once

#include <memory>
#include <assert.h>

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"
#include "Vulkan.h"
#include "Buffer.h"

namespace Engine {
	class BufferHelper {
	public:
		BufferHelper() {};
		~BufferHelper() {};

		template <class T>
		static void CopyFromStaging(
			VulkanEngine& vulkanEngine, 
			const std::vector<T>& content, 
			Buffer& dstBuffer, 
			size_t srcOffset = 0, 
			size_t dstOffset = 0
		);

		template <class T>
		static void CopyFromStaging(
			VulkanEngine& vulkanEngine, 
			const T& content, 
			const VkDeviceSize& contentSize, 
			Buffer* dstBuffer,
			size_t srcOffset = 0,
			size_t dstOffset = 0
		);

		template <class T>
		static void UploadDataToImage(
			VulkanEngine& vulkanEngine,
			const T* imageData,
			const VkDeviceSize& imageSize,
			VkImage& dstImage,
			const uint32_t width,
			const uint32_t height,
			const uint32_t layerCount,
			const uint32_t mipLevel = 0
		);

		template <class T>
		static void AppendData(
			VulkanEngine& vulkanEngine, 
			const std::vector<T>& content, 
			Buffer& buffer, 
			size_t srcOffset, 
			size_t dstOffset
		);

		template <class T>
		static void AppendData(
			VulkanEngine& vulkanEngine, 
			const T& content, 
			const size_t contentSize,
			Buffer& buffer, 
			size_t srcOffset, 
			size_t dstOffset
		);

		template <class T>
		static void CreateBuffer(
			VulkanEngine& vulkanEngine, 
			const VkBufferUsageFlags usageFlags, 
			const VkMemoryPropertyFlags memoryPropertyFlags,
			const T& content, 
			std::unique_ptr<Buffer>& buffer
		);

		template <class T>
		static void CreateBuffer(
			VulkanEngine& vulkanEngine, 
			const VkBufferUsageFlags usageFlags, 
			const VkMemoryPropertyFlags memoryPropertyFlags,
			const std::vector<T>& content, 
			std::unique_ptr<Buffer>& buffer
		);
	};

	template <class T>
	void BufferHelper::CopyFromStaging(
		VulkanEngine& vulkanEngine, 
		const std::vector<T>& content, 
		Buffer& buffer, 
		size_t srcOffset, 
		size_t dstOffset
	) {

		auto bufferSize = sizeof(content[0]) * content.size();
		auto stagingBuffer = std::make_unique<Buffer>(vulkanEngine, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		stagingBuffer->BufferMemory->MapMemory(bufferSize);
		memcpy(stagingBuffer->BufferMemory->MemoryMapped, content.data(), bufferSize);
		stagingBuffer->BufferMemory->UnmapMemory();

		buffer.CopyFrom(stagingBuffer->GetHandle(), bufferSize, srcOffset, dstOffset);

		stagingBuffer.reset();
	}

	template <class T>
	void BufferHelper::CopyFromStaging(
		VulkanEngine& vulkanEngine, 
		const T& content, 
		const VkDeviceSize& contentSize, 
		Buffer* dstBuffer,
		size_t srcOffset,
		size_t dstOffset
	) {

		auto bufferSize = contentSize;
		auto stagingBuffer = std::make_unique<Buffer>(vulkanEngine, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		stagingBuffer->BufferMemory->MapMemory(bufferSize);
		memcpy(stagingBuffer->BufferMemory->MemoryMapped, content, bufferSize);
		stagingBuffer->BufferMemory->UnmapMemory();

		dstBuffer->CopyFrom(stagingBuffer->GetHandle(), bufferSize, srcOffset, dstOffset);

		stagingBuffer.reset();

	}

	template <class T>
	void BufferHelper::UploadDataToImage(
		VulkanEngine& vulkanEngine,
		const T* imageData,
		const VkDeviceSize& imageSize,
		VkImage& dstImage,
		const uint32_t width,
		const uint32_t height,
		const uint32_t layerCount,
		const uint32_t mipLevel
	) {
		if (imageSize == 0) return;

		auto stagingBuffer = std::make_unique<Buffer>(vulkanEngine, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer->BufferMemory->MapMemory(imageSize);
		memcpy(stagingBuffer->BufferMemory->MemoryMapped, imageData, imageSize);
		stagingBuffer->BufferMemory->UnmapMemory();

		VkCommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(vulkanEngine.GetLogicalDevice().GetHandle(),
			vulkanEngine.GetCommandPool().GetHandle());

		const VkBufferImageCopy region = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = VkImageSubresourceLayers {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = mipLevel,
				.baseArrayLayer = 0,
				.layerCount = layerCount
			},
			.imageOffset = VkOffset3D {
				.x = 0,
				.y = 0,
				.z = 0
			},
			.imageExtent = VkExtent3D {
				.width = width,
				.height = height,
				.depth = 1
			}
		};

		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->GetHandle(), dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		CommandBuffer::EndSingleTimeCommandBuffer(vulkanEngine.GetLogicalDevice().GetHandle(), 
			vulkanEngine.GetLogicalDevice().GetGraphicsQueue(),
			commandBuffer, vulkanEngine.GetCommandPool().GetHandle());


		stagingBuffer.reset();
	}

	template <class T>
	void BufferHelper::AppendData(
		VulkanEngine& vulkanEngine,
		const std::vector<T>& content, 
		Buffer& buffer, 
		size_t srcOffset, 
		size_t dstOffset
	) {

		if (content.size() == 0) return;

		CopyFromStaging(vulkanEngine, content, buffer, srcOffset, dstOffset);
	}

	template <class T>
	void BufferHelper::AppendData(VulkanEngine& vulkanEngine, const T& content, const size_t contentSize, Buffer& buffer, size_t srcOffset, size_t dstOffset) {

		if (contentSize == 0) return;

		CopyFromStaging(vulkanEngine, content, contentSize, buffer, srcOffset, dstOffset);
	}

	template <class T>
	void BufferHelper::CreateBuffer(
		VulkanEngine& vulkanEngine,
		const VkBufferUsageFlags usageFlags, 
		const VkMemoryPropertyFlags memoryPropertyFlags,
		const T& content, 
		std::unique_ptr<Buffer>& buffer) {
	
		VkDeviceSize bufferSize = sizeof(T);

		buffer.reset(new class Buffer(vulkanEngine, bufferSize, usageFlags));
		buffer->AllocateMemory(memoryPropertyFlags);

		CopyFromStaging(vulkanEngine, content, buffer.get());
	}

	template <class T>
	void BufferHelper::CreateBuffer(
		VulkanEngine& vulkanEngine,
		const VkBufferUsageFlags usageFlags, 
		const VkMemoryPropertyFlags memoryPropertyFlags,
		const std::vector<T>& content, 
		std::unique_ptr<Buffer>& buffer
	) {
	
		VkDeviceSize bufferSize = sizeof(T) * content.size();

		buffer.reset(new class Buffer(vulkanEngine, bufferSize, usageFlags));
		buffer->AllocateMemory(memoryPropertyFlags);

		CopyFromStaging(vulkanEngine, content, buffer.get());
	}
}
