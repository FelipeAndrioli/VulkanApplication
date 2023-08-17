#pragma once

#include <iostream>
#include <vector>
#include <set>

#include "Vulkan.h"
#include "Common.h"

namespace Engine {
	class PhysicalDevice {
	public:
		PhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);
		~PhysicalDevice();

		VkPhysicalDevice& GetHandle();
		std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();

		inline SwapChainSupportDetails GetSwapChainSupportDetails() { return QuerySwapChainSupportDetails(m_PhysicalDevice); }
	private:
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice device);
	private:
		std::vector<VkPhysicalDevice> m_AvailablePhysicalDevices;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		SwapChainSupportDetails m_SwapChainSupportDetails;

		VkInstance& p_Instance;
		VkSurfaceKHR& p_Surface;
	};
}
