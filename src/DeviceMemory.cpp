#include "DeviceMemory.h"

namespace Engine {
	DeviceMemory::DeviceMemory(VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice)
		: p_LogicalDevice(&logicalDevice), p_PhysicalDevice(&physicalDevice) {

	}

	DeviceMemory::~DeviceMemory() {
		vkFreeMemory(*p_LogicalDevice, Memory, nullptr);
	}

	void DeviceMemory::MapMemory(void* data, const VkDeviceSize dataSize) {
		vkMapMemory(*p_LogicalDevice, Memory, 0, dataSize, 0, &data);
	}

	void DeviceMemory::MapMemory(const VkDeviceSize dataSize) {
		vkMapMemory(*p_LogicalDevice, Memory, 0, dataSize, 0, &MemoryMapped);
	}

	void DeviceMemory::MapMemory(const VkDeviceSize& offset, const VkDeviceSize dataSize) {
		vkMapMemory(*p_LogicalDevice, Memory, offset, dataSize, 0, &MemoryMapped);
	}

	void DeviceMemory::UnmapMemory() {
		vkUnmapMemory(*p_LogicalDevice, Memory);
	}

	void DeviceMemory::AllocateMemory(const VkBuffer& buffer, const VkMemoryPropertyFlags properties) {
		VkMemoryRequirements memRequirements = GetMemoryRequirements(buffer);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size == 0 ? 256 : memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	
		if (vkAllocateMemory(*p_LogicalDevice, &allocInfo, nullptr, &Memory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate buffer memory!");
		}

		vkBindBufferMemory(*p_LogicalDevice, buffer, Memory, 0);
	}

	void DeviceMemory::AllocateMemory(const VkImage& image, VkMemoryPropertyFlags properties) {
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(*p_LogicalDevice, image, &memRequirements);
		
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(*p_LogicalDevice, &allocInfo, nullptr, &Memory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate image memory!");
		}

		vkBindImageMemory(*p_LogicalDevice, image, Memory, 0);
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

	VkMemoryRequirements DeviceMemory::GetMemoryRequirements(const VkBuffer& buffer) {
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(*p_LogicalDevice, buffer, &memRequirements);

		return memRequirements;
	}
}
