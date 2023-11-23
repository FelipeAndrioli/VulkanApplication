#pragma once

#include <vector>

#include "Vulkan.h"
#include "Buffer.h"

namespace Engine {
	class DeviceMemory {
	public:
		DeviceMemory(VkDevice* logicalDevice, VkPhysicalDevice* physicalDevice, int bufferSize);
		~DeviceMemory();

		void AllocateMemory(std::vector<VkBuffer>& buffer, VkMemoryPropertyFlags properties);
		void AllocateMemory(std::vector<VkImage>& images, VkMemoryPropertyFlags properties);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkMemoryRequirements GetMemoryRequirements(VkBuffer& buffer);

		void MapMemory();
		void MapMemory(void* data);
		void UnmapMemory();
	public:
		std::vector<VkDeviceMemory> Memory;
		std::vector<void*> MemoryMapped;

	private:
		VkDevice* p_LogicalDevice = nullptr;
		VkPhysicalDevice* p_PhysicalDevice = nullptr;
	};
}
