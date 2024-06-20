#pragma once

#include <vector>

#include "Vulkan.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "DeviceMemory.h"
#include "CommandBUffer.h"

namespace Engine {
	struct BufferChunk {
		size_t DataSize;
		size_t ChunkSize;
	};

	class Buffer {
	public:
		Buffer(VulkanEngine& vulkanEngine, const size_t bufferSize, const VkBufferUsageFlags usage);
		~Buffer();

		void AllocateMemory(VkMemoryPropertyFlags properties);
		void CopyFrom(VkBuffer srcBuffer, VkDeviceSize bufferSize, size_t srcOffset = 0, size_t dstOffset = 0);

		static void CopyToImage(
			VulkanEngine& vulkanEngine,
			VkQueue& queue,
			VkImage& image,
			const uint32_t imageWidth,
			const uint32_t imageHeight,
			VkImageLayout imageLayout,
			VkBuffer& buffer
		);
		void NewChunk(BufferChunk newChunk);
		void Update(VkDeviceSize offset, void* data, size_t dataSize);

		void* GetMappedMemory();
		inline VkBuffer& GetHandle() { return m_Buffer; };
	public:
		std::vector<BufferChunk> Chunks;
		size_t BufferSize;
		std::unique_ptr<class DeviceMemory> BufferMemory;
	private:
		VkBuffer m_Buffer;
		VulkanEngine* m_VulkanEngine;
	};
}
