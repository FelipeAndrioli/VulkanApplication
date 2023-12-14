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
		Buffer(const int bufferSize, LogicalDevice& logicalDevice, PhysicalDevice& physicalDevice, 
			const size_t size, const VkBufferUsageFlags usage);
		~Buffer();

		void AllocateMemory(VkMemoryPropertyFlags properties);
		void CopyFrom(VkBuffer srcBuffer, VkDeviceSize bufferSize, VkCommandPool& commandPool);
		static void CopyToImage(
			VkDevice& logicalDevice,
			VkCommandPool& commandPool,
			VkQueue& queue,
			VkImage& image,
			const uint32_t imageWidth,
			const uint32_t imageHeight,
			VkBuffer& buffer
		);

		VkBuffer& GetBuffer(uint32_t index);
		void* GetBufferMemoryMapped(uint32_t index);

		inline VkBuffer& GetBuffer() { return m_Buffer[0]; };

	public:
		std::unique_ptr<class DeviceMemory> BufferMemory;
	private:
		std::vector<VkBuffer> m_Buffer;

		LogicalDevice* p_LogicalDevice;

		int m_BufferSize;
	};
}
