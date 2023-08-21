#pragma once

#include <iostream>
#include <vector>
#include <set>

#include "Vulkan.h"
#include "Instance.h"
#include "PhysicalDevice.h"

namespace Engine {
	class LogicalDevice {
	public:
		LogicalDevice(Instance* instance, PhysicalDevice* physicalDevice);
		~LogicalDevice();

		inline VkDevice& GetHandle() { return m_VulkanDevice; };
		inline VkQueue& GetGraphicsQueue() { return m_GraphicsQueue; };
		inline VkQueue& GetPresentQueue() { return m_PresentQueue; };
		inline VkQueue& GetComputeQueue() { return m_ComputeQueue; };
	private:
		VkDevice m_VulkanDevice = VK_NULL_HANDLE;

		Instance* p_Instance;
		PhysicalDevice* p_PhysicalDevice;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		VkQueue m_ComputeQueue;
	};
}
