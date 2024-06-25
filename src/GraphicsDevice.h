#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include <string>
#include <set>
#include <assert.h>

#include "VulkanHeader.h"

namespace PhysicalDevice {

	const int DEDICATED_GPU = 2;
	const std::vector<const char*> c_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t>graphicsFamily;
		std::optional<uint32_t>presentFamily;
		std::optional<uint32_t>graphicsAndComputeFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value() && graphicsAndComputeFamily.has_value();
		}
	};

	VkPhysicalDevice CreatePhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);
	SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice& device, VkSurfaceKHR& surface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);
	bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	VkSampleCountFlagBits GetMaxSampleCount(VkPhysicalDeviceProperties deviceProperties);
		
	std::vector<VkPhysicalDevice> m_AvailablePhysicalDevices;
}

namespace Instance {
#ifndef NDEBUG
	const bool c_EnableValidationLayers = true;
#else
	const bool c_EnableValidationLayers = false;
#endif

	const std::vector<const char*> c_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	void CreateInstance(VkInstance& instance);
	bool checkValidationLayerSupport();
}

namespace Surface {
	void CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface);
}

namespace LogicalDevice {
	void CreateLogicalDevice(PhysicalDevice::QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice);
	void CreateQueue(VkDevice& logicalDevice, uint32_t queueFamilyIndex, VkQueue& queue);
	void WaitIdle(VkDevice& logicalDevice);
}

namespace DebugMessenger {
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void CreateDebugMessenger(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger);
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
}

namespace Engine {
	
	class GraphicsDevice {
	public:
		GraphicsDevice(GLFWwindow& window);
		~GraphicsDevice();

	private:
		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		PhysicalDevice::SwapChainSupportDetails m_SwapChainSupportDetails;
		PhysicalDevice::QueueFamilyIndices m_QueueFamilyIndices;
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		VkQueue m_ComputeQueue;


	};
}