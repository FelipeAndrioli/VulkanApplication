#include "PhysicalDevice.h"

namespace Engine {
	PhysicalDevice::PhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface) 
		: p_Instance(instance), p_Surface(surface) {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(p_Instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		m_AvailablePhysicalDevices.resize(deviceCount);
		vkEnumeratePhysicalDevices(p_Instance, &deviceCount, m_AvailablePhysicalDevices.data());

		std::cout << "Available Devices:" << '\n';

		for (const auto& device : m_AvailablePhysicalDevices) {
			VkPhysicalDeviceFeatures device_features;
			VkPhysicalDeviceProperties device_properties;
	
			vkGetPhysicalDeviceFeatures(device, &device_features);
			vkGetPhysicalDeviceProperties(device, &device_properties);

			std::cout << '\t' << device_properties.deviceName << '\n';
			std::cout << '\t' << device_properties.deviceType << '\n';
		}
		
		for (const auto& device : m_AvailablePhysicalDevices) {
			if (isDeviceSuitable(device)) {
				
				VkPhysicalDeviceProperties deviceProperties;
				vkGetPhysicalDeviceProperties(device, &deviceProperties);

				// pick any GPU at first, then priorize a dedicated GPU (device type = 2)
				if (m_PhysicalDevice == VK_NULL_HANDLE || deviceProperties.deviceType == 2) {
					m_PhysicalDevice = device;
					m_PhysicalDeviceProperties = deviceProperties;
				}
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU!");
		} else {
			std::cout << "Selected device: " << m_PhysicalDeviceProperties.deviceName << '\n';
		}

		m_SwapChainSupportDetails = QuerySwapChainSupportDetails(m_PhysicalDevice);
		m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);
	}

	PhysicalDevice::~PhysicalDevice() {

	}

	VkPhysicalDeviceLimits PhysicalDevice::GetLimits() {
		return m_PhysicalDeviceProperties.limits;
	}

	SwapChainSupportDetails PhysicalDevice::QuerySwapChainSupportDetails(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, p_Surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_Surface, &formatCount, details.formats.data());

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_Surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_Surface, &presentModeCount, details.presentModes.data());

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_Surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	QueueFamilyIndices PhysicalDevice::FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
				indices.graphicsFamily = i;
				indices.graphicsAndComputeFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, p_Surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}

	bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = FindQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;

		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupportDetails(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(c_DeviceExtensions.begin(), c_DeviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}
}