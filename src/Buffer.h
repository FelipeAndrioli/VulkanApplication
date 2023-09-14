#pragma once

#include <vector>

#include "Vulkan.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

namespace Engine {
	class Buffer {
	public:
		Buffer(const int bufferSize, LogicalDevice* logicalDevice, PhysicalDevice* physicalDevice, 
			const size_t size, const VkBufferUsageFlags usage);
		~Buffer();

		void AllocateMemory(VkMemoryPropertyFlags properties);
		void MapMemory();
		void MapMemory(void* data);
		void UnmapMemory();
		void CopyFrom(VkBuffer srcBuffer, VkDeviceSize bufferSize, VkCommandPool& commandPool);

		VkBuffer& GetBuffer(uint32_t index);
		VkDeviceMemory& GetBufferMemory(uint32_t index);
		void* GetBufferMemoryMapped(uint32_t index);

		inline VkBuffer& GetBuffer() { return m_Buffer[0]; };
	private:
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkMemoryRequirements GetMemoryRequirements(VkBuffer& buffer);
	private:
		std::vector<VkBuffer> m_Buffer;
		std::vector<VkDeviceMemory> m_BufferMemory;
		std::vector<void*> m_BufferMemoryMapped;

		LogicalDevice* p_LogicalDevice;
		PhysicalDevice* p_PhysicalDevice;

		int m_BufferSize;
	};
}
