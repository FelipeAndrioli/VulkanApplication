#pragma once

#include <vector>

#include "Vulkan.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "DeviceMemory.h"
#include "CommandBUffer.h"

namespace Engine {
	class Buffer {
	public:
		Buffer(const int numBuffers, LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, 
			const size_t bufferSize, const VkBufferUsageFlags usage);
		~Buffer();

		void AllocateMemory(VkMemoryPropertyFlags properties);
		void CopyFrom(VkBuffer srcBuffer, VkDeviceSize bufferSize, VkCommandPool& commandPool, size_t srcOffset = 0, size_t dstOffset = 0);
		static void CopyToImage(
			VkDevice& logicalDevice,
			VkCommandPool& commandPool,
			VkQueue& queue,
			VkImage& image,
			const uint32_t imageWidth,
			const uint32_t imageHeight,
			VkImageLayout imageLayout,
			VkBuffer& buffer
		);
		void AppendData();

		VkBuffer& GetBuffer(uint32_t index);
		void* GetBufferMemoryMapped(uint32_t index);
		inline VkBuffer& GetBuffer() { return m_Buffer[0]; };
	public:
		std::vector<size_t> DataSizes;
		std::vector<size_t> ChunkSizes;
		size_t BufferSize;
		std::unique_ptr<class DeviceMemory> BufferMemory;
	private:
		int m_NumBuffers;
		std::vector<VkBuffer> m_Buffer;

		LogicalDevice* p_LogicalDevice;
	};
}
