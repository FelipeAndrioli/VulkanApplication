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

		inline VkPhysicalDevice& GetHandle() { return m_PhysicalDevice; };
		inline std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices() { return m_AvailablePhysicalDevices; };
		inline SwapChainSupportDetails GetSwapChainSupportDetails() { return QuerySwapChainSupportDetails(m_PhysicalDevice); }
		inline QueueFamilyIndices& GetQueueFamilyIndices() { return m_QueueFamilyIndices; }
		VkPhysicalDeviceLimits GetLimits();
		const VkSampleCountFlagBits GetMsaaSamples() { return m_MsaaSamples; }
	private:
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice device);
		static VkSampleCountFlagBits GetMaxSampleCount(VkPhysicalDeviceProperties deviceProperties);
	private:
		std::vector<VkPhysicalDevice> m_AvailablePhysicalDevices;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		SwapChainSupportDetails m_SwapChainSupportDetails;

		VkInstance& p_Instance;
		VkSurfaceKHR& p_Surface;
		QueueFamilyIndices m_QueueFamilyIndices;
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

		const int DEDICATED_GPU = 2;
	};
}
