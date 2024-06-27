#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include <string>
#include <set>
#include <algorithm>
#include <assert.h>

#include "VulkanHeader.h"
#include "Window.h"

namespace Engine::Graphics {
	const int DEDICATED_GPU = 2;
	const std::vector<const char*> c_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
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
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);
	bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	VkSampleCountFlagBits GetMaxSampleCount(VkPhysicalDeviceProperties deviceProperties);

	std::vector<VkPhysicalDevice> m_AvailablePhysicalDevices;

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

	void CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface);

	void CreateLogicalDevice(QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice);
	void CreateQueue(VkDevice& logicalDevice, uint32_t queueFamilyIndex, VkQueue& queue);
	void WaitIdle(VkDevice& logicalDevice);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void CreateDebugMessenger(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger);
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct SwapChain {
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		VkFormat swapChainImageFormat;

		VkExtent2D swapChainExtent;
	};

	void CreateSwapChain(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
	void CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
	void CreateSwapChainImageViews(VkDevice& logicalDevice, SwapChain& swapChain);
	void DestroySwapChain(VkDevice& logicalDevice, SwapChain& swapChain);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>&availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, const VkExtent2D & extent);

	class GraphicsDevice {
	public:
		GraphicsDevice(Window& window);
		~GraphicsDevice();

		bool CreateSwapChain(Window& window, SwapChain& swapChain);

		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		QueueFamilyIndices m_QueueFamilyIndices;
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		VkQueue m_ComputeQueue;
	};
}