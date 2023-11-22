#include "DeviceMemory.h"

namespace Engine {
	DeviceMemory::DeviceMemory(VkDevice* logicalDevice, VkPhysicalDevice* physicalDevice, int bufferSize) 
		: p_LogicalDevice(logicalDevice), p_PhysicalDevice(physicalDevice) {

		Memory.resize(bufferSize);
		MemoryMapped.resize(bufferSize);
	}
	
	DeviceMemory::~DeviceMemory() {
		for (size_t i = 0; i < Memory.size(); i++) {
			vkFreeMemory(*p_LogicalDevice, Memory[i], nullptr);
		}
	}

	void DeviceMemory::MapMemory() {
		for (size_t i = 0; i < Memory.size(); i++) {
			vkMapMemory(*p_LogicalDevice, Memory[i], 0, Memory.size(), 0, &MemoryMapped[i]);
		}
	}

	void DeviceMemory::MapMemory(void* data) {
		for (size_t i = 0; i < Memory.size(); i++) {
			vkMapMemory(*p_LogicalDevice, Memory[i], 0, Memory.size(), 0, &data);
		}
	}

	void DeviceMemory::UnmapMemory() {
		for (size_t i = 0; i < Memory.size(); i++) {
			vkUnmapMemory(*p_LogicalDevice, Memory[i]);
		}
	}

	void DeviceMemory::AllocateMemory(std::vector<VkBuffer>& buffer, VkMemoryPropertyFlags properties) {
		for (size_t i = 0; i < buffer.size(); i++) {
			VkMemoryRequirements memRequirements = GetMemoryRequirements(buffer[i]);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
		
			if (vkAllocateMemory(*p_LogicalDevice, &allocInfo, nullptr, &Memory[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to allocate buffer memory!");
			}

			vkBindBufferMemory(*p_LogicalDevice, buffer[i], Memory[i], 0);
		}
	}

	uint32_t DeviceMemory::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(*p_PhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	VkMemoryRequirements DeviceMemory::GetMemoryRequirements(VkBuffer& buffer) {
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(*p_LogicalDevice, buffer, &memRequirements);

		return memRequirements;
	}
}
