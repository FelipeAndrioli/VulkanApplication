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
		static void CopyFromStaging(LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, CommandPool& commandPool, 
			const std::vector<T>& content, Buffer& dstBuffer, size_t srcOffset = 0, size_t dstOffset = 0);

		template <class T>
		static void CopyFromStaging(LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, 
			VkCommandPool& commandPool, const T& content, const VkDeviceSize& contentSize, Buffer* dstBuffer);

		template <class T>
		static void AppendData(LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, CommandPool& commandPool,
			const std::vector<T>& content, Buffer& buffer, size_t srcOffset, size_t dstOffset);

		template <class T>
		static void CreateBuffer(
			const int bufferQuantity, 
			LogicalDevice& logicalDevice, 
			PhysicalDevice& physicalDevice,
			CommandPool& commandPool, 
			const VkBufferUsageFlags usageFlags, 
			const VkMemoryPropertyFlags memoryPropertyFlags,
			const T& content, 
			std::unique_ptr<Buffer>& buffer
		);

		template <class T>
		static void CreateBuffer(
			const int numBuffer, 
			LogicalDevice& logicalDevice, 
			PhysicalDevice& physicalDevice,
			CommandPool& commandPool, 
			const VkBufferUsageFlags usageFlags, 
			const VkMemoryPropertyFlags memoryPropertyFlags,
			const std::vector<T>& content, 
			std::unique_ptr<Buffer>& buffer
		);
	};

	template <class T>
	void BufferHelper::CopyFromStaging(LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, CommandPool& commandPool, 
		const std::vector<T>& content, Buffer& buffer, size_t srcOffset, size_t dstOffset) {

		auto bufferSize = sizeof(content[0]) * content.size();
		auto stagingBuffer = std::make_unique<Buffer>(1, logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		stagingBuffer->BufferMemory->MapMemory();
		memcpy(stagingBuffer->BufferMemory->MemoryMapped[0], content.data(), bufferSize);
		stagingBuffer->BufferMemory->UnmapMemory();

		buffer.CopyFrom(stagingBuffer->GetBuffer(0), bufferSize, commandPool.GetHandle(), srcOffset, dstOffset);

		stagingBuffer.reset();
	}

	template <class T>
	void BufferHelper::CopyFromStaging(LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, VkCommandPool& commandPool,
		const T& content, const VkDeviceSize& contentSize, Buffer* dstBuffer) {

		auto bufferSize = contentSize;
		auto stagingBuffer = std::make_unique<Buffer>(1, logicalDevice, physicalDevice, 
			bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		stagingBuffer->BufferMemory->MapMemory();
		memcpy(stagingBuffer->BufferMemory->MemoryMapped[0], content, bufferSize);
		stagingBuffer->BufferMemory->UnmapMemory();

		dstBuffer->CopyFrom(stagingBuffer->GetBuffer(0), bufferSize, commandPool);

		stagingBuffer.reset();

	}

	template <class T>
	void BufferHelper::AppendData(LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, CommandPool& commandPool,
		const std::vector<T>& content, Buffer& buffer, size_t srcOffset, size_t dstOffset) {

		buffer.DataSizes.push_back(sizeof(T));
		buffer.ChunkSizes.push_back(sizeof(T) * content.size());
		CopyFromStaging(logicalDevice, physicalDevice, commandPool, content, buffer, srcOffset, dstOffset);

		/*
		VkDeviceSize bufferSize = sizeof(T) * content.size();
		auto stagingBuffer = std::make_unique<Buffer>(1, logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer->BufferMemory->MapMemory();
		memcpy(stagingBuffer->BufferMemory->MemoryMapped[0], content.data(), bufferSize);
		stagingBuffer->BufferMemory->UnmapMemory();
		buffer.CopyFrom(stagingBuffer->GetBuffer(0), bufferSize, commandPool.GetHandle(), srcOffset, dstOffset);
		stagingBuffer.reset();
		*/
	}

	template <class T>
	static void BufferHelper::CreateBuffer(
		const int bufferQuantity, 
		LogicalDevice& logicalDevice, 
		PhysicalDevice& physicalDevice,
		CommandPool& commandPool, 
		const VkBufferUsageFlags usageFlags, 
		const VkMemoryPropertyFlags memoryPropertyFlags,
		const T& content, 
		std::unique_ptr<Buffer>& buffer
	) {
	
		VkDeviceSize bufferSize = sizeof(T);

		buffer.reset(new class Buffer(
			bufferQuantity,
			logicalDevice,
			physicalDevice,
			bufferSize,
			usageFlags
		));
		buffer->AllocateMemory(memoryPropertyFlags);

		CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(), content, buffer.get());
	}

	template <class T>
	static void BufferHelper::CreateBuffer(
		const int numBuffer, 
		LogicalDevice& logicalDevice, 
		PhysicalDevice& physicalDevice,
		CommandPool& commandPool, 
		const VkBufferUsageFlags usageFlags, 
		const VkMemoryPropertyFlags memoryPropertyFlags,
		const std::vector<T>& content, 
		std::unique_ptr<Buffer>& buffer
	) {
	
		VkDeviceSize bufferSize = sizeof(T) * content.size();

		buffer.reset(new class Buffer(
			numBuffer,
			logicalDevice,
			physicalDevice,
			bufferSize,
			usageFlags
		));
		buffer->AllocateMemory(memoryPropertyFlags);

		CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(), content, buffer.get());
	}
}
