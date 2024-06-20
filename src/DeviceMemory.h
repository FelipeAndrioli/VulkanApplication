#pragma once

#include <vector>

#include "Vulkan.h"
#include "Buffer.h"

namespace Engine {
	class DeviceMemory {
	public:
		DeviceMemory(VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice);
		~DeviceMemory();

		void AllocateMemory(const VkBuffer& buffer, const VkMemoryPropertyFlags properties);
		void AllocateMemory(const VkImage& image, const VkMemoryPropertyFlags properties);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkMemoryRequirements GetMemoryRequirements(const VkBuffer& buffer);

		void MapMemory(void* data, const VkDeviceSize dataSize);
		void MapMemory(const VkDeviceSize dataSize);
		void MapMemory(const VkDeviceSize& offset, const VkDeviceSize dataSize);
		void UnmapMemory();
	public:
		VkDeviceMemory Memory;
		void* MemoryMapped;

	private:
		VkDevice* p_LogicalDevice = nullptr;
		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
