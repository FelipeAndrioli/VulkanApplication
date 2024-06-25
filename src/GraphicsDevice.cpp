#include "GraphicsDevice.h"

namespace PhysicalDevice {
	VkPhysicalDevice CreatePhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface) {

		assert(instance != VK_NULL_HANDLE);
		assert(surface != VK_NULL_HANDLE);

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		assert(deviceCount != 0, "A GPU with Vulkan support is required!");

		m_AvailablePhysicalDevices.resize(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, m_AvailablePhysicalDevices.data());

		std::cout << "Available Devices:" << '\n';

		for (const auto& device : m_AvailablePhysicalDevices) {
			VkPhysicalDeviceFeatures device_features;
			VkPhysicalDeviceProperties device_properties;

			vkGetPhysicalDeviceFeatures(device, &device_features);
			vkGetPhysicalDeviceProperties(device, &device_properties);

			std::cout << '\t' << device_properties.deviceName << '\n';
		}

		for (const auto& device : m_AvailablePhysicalDevices) {
			if (!isDeviceSuitable(device, surface))
				continue;

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			// pick any GPU at first, then priorize a dedicated GPU (device type = 2)
			if (physicalDevice == VK_NULL_HANDLE || deviceProperties.deviceType == DEDICATED_GPU) {
				physicalDevice = device;
			}
		}

		assert(physicalDevice != VK_NULL_HANDLE);

		return physicalDevice;
	}

	SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

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

	bool isDeviceSuitable(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
		QueueFamilyIndices indices = FindQueueFamilies(device, surface);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;

		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupportDetails(device, surface);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

	VkSampleCountFlagBits GetMaxSampleCount(VkPhysicalDeviceProperties deviceProperties) {
		VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	VkPhysicalDeviceProperties GetDeviceProperties(VkPhysicalDevice& physicalDevice) {
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		return deviceProperties;
	}
}

namespace Surface {
	void CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface) {
		assert(instance != VK_NULL_HANDLE, "Instance must not be VK_NULL_HANDLE");
		assert(surface == VK_NULL_HANDLE, "Surface must be VK_NULL_HANDLE");
		VkResult result = glfwCreateWindowSurface(instance, &window, nullptr, &surface);

		assert(result == VK_SUCCESS);
	}
}

namespace Instance {
	void CreateInstance(VkInstance& instance) {

		assert(instance == VK_NULL_HANDLE, "Instance must be VK_NULL_HANDLE!");

		if (c_EnableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "VulkanApplication.exe";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto glfwExtensions = getRequiredExtensions();

		createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
		createInfo.ppEnabledExtensionNames = glfwExtensions.data();

		if (c_EnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(c_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = c_ValidationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create instance!");
		}

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "Available extensions: \n";

		for (const auto& extension : extensions) {
			std::cout << '\t' << extension.extensionName << '\n';
		}

		checkRequiredExtensions(static_cast<uint32_t>(glfwExtensions.size()), glfwExtensions.data(),
			extensions);
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : c_ValidationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	void checkRequiredExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions, 
		std::vector<VkExtensionProperties> vulkanSupportedExtensions) {

		bool found = false;

		for (uint32_t i = 0; i < glfwExtensionCount; i++) {
			for (const auto& extension : vulkanSupportedExtensions) {
				if (strcmp(glfwExtensions[i], extension.extensionName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				throw std::runtime_error("Required extension not supported!");
			}
			else {
				found = false;
				continue;
			}
		}
	}

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (c_EnableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}
}

namespace LogicalDevice {
	void CreateLogicalDevice(PhysicalDevice::QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice) {
		assert(logicalDevice == VK_NULL_HANDLE, "Logical device must be VK_NULL_HANDLE");
		assert(physicalDevice != VK_NULL_HANDLE, "Physical device must not be VK_NULL_HANDLE");

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamilies = { 
			indices.graphicsFamily.value(), 
			indices.presentFamily.value(), 
			indices.graphicsAndComputeFamily.value() 
		};

		float queuePriority = 1.0f;

		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.wideLines = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		//createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(PhysicalDevice::c_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = PhysicalDevice::c_DeviceExtensions.data();

		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingCreateInfo = {};
		descriptorIndexingCreateInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexingCreateInfo.runtimeDescriptorArray = VK_TRUE;

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.features = deviceFeatures;
		deviceFeatures2.pNext = &descriptorIndexingCreateInfo;

		createInfo.pNext = &deviceFeatures2;

		if (Instance::c_EnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(Instance::c_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = Instance::c_ValidationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);

		assert(result == VK_SUCCESS);
	}

	void CreateQueue(VkDevice& logicalDevice, uint32_t queueFamilyIndex, VkQueue& queue) {
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &queue);
	}

	void WaitIdle(VkDevice& logicalDevice) {
		vkDeviceWaitIdle(logicalDevice);
	}
}

namespace DebugMessenger {
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
		}

		return VK_FALSE;
	}

	void CreateDebugMessenger(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger) {
		assert(instance != VK_NULL_HANDLE);
		assert(debugMessenger == VK_NULL_HANDLE);

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);

		assert(result == VK_SUCCESS);
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // optional
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, 
		const VkAllocationCallbacks* pAllocator) {
	
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}
}

namespace Engine {
	
	GraphicsDevice::GraphicsDevice(GLFWwindow& window) {
		Instance::CreateInstance(m_VulkanInstance);
		Surface::CreateSurface(m_VulkanInstance, window, m_Surface);
		m_PhysicalDevice = PhysicalDevice::CreatePhysicalDevice(m_VulkanInstance, m_Surface);

		m_PhysicalDeviceProperties = PhysicalDevice::GetDeviceProperties(m_PhysicalDevice);

		std::cout << "Selected device: " << m_PhysicalDeviceProperties.deviceName << '\n';
		std::cout << "MSAA Samples: " << m_MsaaSamples << '\n';

		m_MsaaSamples = PhysicalDevice::GetMaxSampleCount(m_PhysicalDeviceProperties);
		m_SwapChainSupportDetails = PhysicalDevice::QuerySwapChainSupportDetails(m_PhysicalDevice, m_Surface);
		m_QueueFamilyIndices = PhysicalDevice::FindQueueFamilies(m_PhysicalDevice, m_Surface);

		LogicalDevice::CreateLogicalDevice(m_QueueFamilyIndices, m_PhysicalDevice, m_LogicalDevice);
		LogicalDevice::CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.graphicsFamily.value(), m_GraphicsQueue);
		LogicalDevice::CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.presentFamily.value(), m_PresentQueue);
		LogicalDevice::CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.graphicsAndComputeFamily.value(), m_ComputeQueue);

		DebugMessenger::CreateDebugMessenger(m_VulkanInstance, m_DebugMessenger);
	}

	GraphicsDevice::~GraphicsDevice() {
		DebugMessenger::DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
		vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
	}
}