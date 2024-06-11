#pragma once

#include <vector>

#include "Vulkan.h"
#include "Buffer.h"

namespace Engine {
	class DeviceMemory {
	public:
		DeviceMemory(VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice, int numMemory);
		~DeviceMemory();

		void AllocateMemory(std::vector<VkBuffer>& buffer, VkMemoryPropertyFlags properties);
		void AllocateMemory(VkImage& image, VkMemoryPropertyFlags properties);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkMemoryRequirements GetMemoryRequirements(VkBuffer& buffer);

		void MapMemory(void* data, const VkDeviceSize dataSize);
		void MapMemory(const VkDeviceSize dataSize);
		void MapMemory(uint32_t index, const VkDeviceSize& offset, const VkDeviceSize dataSize);
		void UnmapMemory();
		void UnmapMemory(uint32_t index);
	public:
		std::vector<VkDeviceMemory> Memory;
		std::vector<void*> MemoryMapped;

	private:
		VkDevice* p_LogicalDevice = nullptr;
		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
