#include "GraphicsDevice.h"

namespace Engine::Graphics {
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

	void CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface) {
		assert(instance != VK_NULL_HANDLE, "Instance must not be VK_NULL_HANDLE");
		assert(surface == VK_NULL_HANDLE, "Surface must be VK_NULL_HANDLE");

		VkResult result = glfwCreateWindowSurface(instance, &window, nullptr, &surface);

		assert(result == VK_SUCCESS);
	}

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

	void CreateLogicalDevice(QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice) {
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

		createInfo.enabledExtensionCount = static_cast<uint32_t>(c_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = c_DeviceExtensions.data();

		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingCreateInfo = {};
		descriptorIndexingCreateInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexingCreateInfo.runtimeDescriptorArray = VK_TRUE;

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.features = deviceFeatures;
		deviceFeatures2.pNext = &descriptorIndexingCreateInfo;

		createInfo.pNext = &deviceFeatures2;

		if (c_EnableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(c_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = c_ValidationLayers.data();
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

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& extent) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			VkExtent2D actualExtent = extent;

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height , capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupportDetails(physicalDevice, surface);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, currentExtent);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			// VK_SHARING_MODE_CONCURRENT - Images can be used across multiple queue familes without 
			// explicit ownership transfers.

			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			// VK_SHARING_MODE_EXCLUSIVE - An image is owned by one queue family at a time and ownership 
			// must be explicitly transferred before using it in another queue family. This option offers 
			// the best performance

			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain.swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(logicalDevice, swapChain.swapChain, &imageCount, nullptr);
		swapChain.swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain.swapChain, &imageCount, swapChain.swapChainImages.data());
		swapChain.swapChainImageFormat = surfaceFormat.format;
		swapChain.swapChainExtent = extent;
	}

	void CreateSwapChainImageViews(VkDevice& logicalDevice, SwapChain& swapChain) {
		swapChain.swapChainImageViews.resize(swapChain.swapChainImages.size());

		for (size_t i = 0; i < swapChain.swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChain.swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChain.swapChainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChain.swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image views");
			}
		}
	}

	void DestroySwapChain(VkDevice& logicalDevice, SwapChain& swapChain) {
		for (auto imageView : swapChain.swapChainImageViews) {
			vkDestroyImageView(logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain.swapChain, nullptr);
	}

	void WaitIdle(VkDevice& logicalDevice) {
		vkDeviceWaitIdle(logicalDevice);
	}

	void RecreateSwapChain(Window& window, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain) {
		VkExtent2D currentExtent = window.GetFramebufferSize();

		while (currentExtent.width == 0 || currentExtent.height == 0) {
			currentExtent = window.GetFramebufferSize();
			window.WaitEvents();
		}

		WaitIdle(logicalDevice);

		DestroySwapChain(logicalDevice, swapChain);
		CreateSwapChainInternal(physicalDevice, logicalDevice, surface, swapChain, window.GetFramebufferSize());
		CreateSwapChainImageViews(logicalDevice, swapChain);
		CreateSwapChainSemaphores(logicalDevice, swapChain);
	}

	void CreateSwapChainSemaphores(VkDevice& logicalDevice, SwapChain& swapChain) {

		swapChain.imageAvailableSemaphores.clear();
		swapChain.renderFinishedSemaphores.clear();

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (size_t i = 0; i < swapChain.swapChainImages.size(); i++) {
			VkResult result = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &swapChain.imageAvailableSemaphores.emplace_back());
			assert(result == VK_SUCCESS);

			result = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &swapChain.renderFinishedSemaphores.emplace_back());
			assert(result == VK_SUCCESS);
		}
	}

	void CreateSwapChain(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent) {
		CreateSwapChainInternal(physicalDevice, logicalDevice, surface, swapChain, currentExtent);
		CreateSwapChainImageViews(logicalDevice, swapChain);
		CreateSwapChainSemaphores(logicalDevice, swapChain);
	}

	void CreateCommandPool(VkDevice& logicalDevice, VkCommandPool& commandPool, uint32_t queueFamilyIndex) {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndex;

		VkResult result = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool);
		assert(result == VK_SUCCESS);
	}

	void BeginCommandBuffer(VkDevice& logicalDevice, VkCommandPool& commandPool, VkCommandBuffer& commandBuffer) {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		assert(result == VK_SUCCESS);
	}

	void EndCommandBuffer(VkCommandBuffer& commandBuffer) {
		VkResult result = vkEndCommandBuffer(commandBuffer);
		assert(result == VK_SUCCESS);
	}

	VkCommandBuffer BeginSingleTimeCommandBuffer(VkDevice& logicalDevice, VkCommandPool& commandPool) {
		assert(logicalDevice != VK_NULL_HANDLE);
		assert(commandPool != VK_NULL_HANDLE);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = {};
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		assert(result == VK_SUCCESS);

		return commandBuffer;
	}

	void EndSingleTimeCommandBuffer(VkDevice& logicalDevice, VkQueue& queue, VkCommandBuffer& commandBuffer, VkCommandPool& commandPool) {

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
	}

	uint32_t FindMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	VkMemoryRequirements GetMemoryRequirements(VkDevice& logicalDevice, const VkBuffer& buffer) {
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

		return memRequirements;
	}

	void CreateImage(VkDevice& logicalDevice, GPUImage& image) {
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = image.ImageType;
		imageCreateInfo.extent.width = image.Width;
		imageCreateInfo.extent.height = image.Height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = image.MipLevels;
		imageCreateInfo.arrayLayers = (uint32_t)(image.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? 6 : 1);

		if (image.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
			imageCreateInfo.flags = image.AspectFlags;
		}

		imageCreateInfo.format = image.Format;
		imageCreateInfo.tiling = image.Tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = image.Usage;
		imageCreateInfo.samples = image.MsaaSamples;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image.Image);

		assert(result == VK_SUCCESS);
	}

	void CreateImageView(VkDevice& logicalDevice, GPUImage& image) {
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = image.Image;
		viewCreateInfo.viewType = image.ViewType;
		viewCreateInfo.format = image.Format;
		viewCreateInfo.subresourceRange.aspectMask = image.AspectFlags;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = image.MipLevels;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = image.LayerCount;

		if (vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &image.ImageView) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Image View!");
		}
	}

	void TransitionImageLayout(VkDevice& logicalDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue, GPUImage& image, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(logicalDevice, commandPool);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = image.ImageLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image.Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = image.MipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = image.LayerCount;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags dstStage;

		if (image.ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (image.ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::invalid_argument("Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage,  
			dstStage, 
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		CommandBuffer::EndSingleTimeCommandBuffer(logicalDevice, graphicsQueue, commandBuffer, commandPool);

		image.ImageLayout = newLayout;
	}

	void GenerateImageMipMaps(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkCommandPool& commandPool, VkQueue& queue, GPUImage& image) {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, image.Format, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("Texture image format does not support linear blitting!");
		}

		VkCommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(logicalDevice, commandPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image.Image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = image.Width;
		int32_t mipHeight = image.Height;

		for (uint32_t i = 1; i < image.MipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer, image.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = image.MipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);		

		CommandBuffer::EndSingleTimeCommandBuffer(logicalDevice, queue, commandBuffer, commandPool);
		
		image.ImageLayout = barrier.newLayout;
	}

	void AllocateMemory(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, GPUImage& image) {
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, image.Image, &memRequirements);
		
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, image.MemoryProperty);

		VkResult result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &image.Memory);

		assert(result == VK_SUCCESS);

		vkBindImageMemory(logicalDevice, image.Image, image.Memory, 0);
	}

	void AllocateMemory(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, GPUBuffer& buffer) {
		VkMemoryRequirements memRequirements = GetMemoryRequirements(logicalDevice, buffer.Handle);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size == 0 ? 256 : memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, buffer.MemoryProperty);

		VkResult result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &buffer.Memory);

		assert(result == VK_SUCCESS);

		vkBindBufferMemory(logicalDevice, buffer.Handle, buffer.Memory, 0);
	}

	void CreateImageSampler(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, GPUImage& image, VkSamplerAddressMode addressMode) {
		// TODO: add parameters as variables
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(image.MipLevels);

		VkResult result = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &image.ImageSampler);

		assert(result == VK_SUCCESS);
	}

	void CopyBufferToImage(VkDevice& logicalDevice, VkCommandPool& commandPool, VkQueue& queue, GPUImage& image, VkBuffer& srcBuffer) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(logicalDevice, commandPool);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0; // TODO: test with mip level
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { image.Width, image.Height, 1 }; 

		vkCmdCopyBufferToImage(commandBuffer, srcBuffer, image.Image, image.ImageLayout, 1, &region);

		CommandBuffer::EndSingleTimeCommandBuffer(logicalDevice, queue, commandBuffer, commandPool);
	}

	void DestroyImage(VkDevice& logicalDevice, GPUImage& image) {
		vkDestroyImageView(logicalDevice, image.ImageView, nullptr);
		vkDestroyImage(logicalDevice, image.Image, nullptr);
		vkFreeMemory(logicalDevice, image.Memory, nullptr);
	}

	VkFormat FindDepthFormat(VkPhysicalDevice& physicalDevice) {
		return FindSupportedFormat(physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat FindSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
		VkFormatFeatureFlags features) {

		for (VkFormat format : candidates) {
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("Failed to find supported format!");
	}

	bool HasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void CreateBuffer(VkDevice& logicalDevice, GPUBuffer& buffer) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = buffer.BufferSize == 0 ? 256 : buffer.BufferSize;
		bufferInfo.usage = buffer.Usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer.Handle);

		assert(result == VK_SUCCESS);
	}

	void CopyBuffer(VkDevice& logicalDevice, VkCommandPool& commandPool, VkQueue& queue, VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size, size_t srcOffset, size_t dstOffset) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(logicalDevice, commandPool);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommandBuffer(logicalDevice, queue, commandBuffer, commandPool);
	}

	void UpdateBuffer() {

	}

	GraphicsDevice::GraphicsDevice(Window& window) {
		CreateInstance(m_VulkanInstance);
		CreateSurface(m_VulkanInstance, *window.GetHandle(), m_Surface);
		m_PhysicalDevice = CreatePhysicalDevice(m_VulkanInstance, m_Surface);

		m_PhysicalDeviceProperties = GetDeviceProperties(m_PhysicalDevice);

		std::cout << "Selected device: " << m_PhysicalDeviceProperties.deviceName << '\n';
		std::cout << "MSAA Samples: " << m_MsaaSamples << '\n';

		m_MsaaSamples = GetMaxSampleCount(m_PhysicalDeviceProperties);
		m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, m_Surface);

		CreateLogicalDevice(m_QueueFamilyIndices, m_PhysicalDevice, m_LogicalDevice);
		CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.graphicsFamily.value(), m_GraphicsQueue);
		CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.presentFamily.value(), m_PresentQueue);
		CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.graphicsAndComputeFamily.value(), m_ComputeQueue);

		CreateDebugMessenger(m_VulkanInstance, m_DebugMessenger);
		CreateFramesResources();
		CreateCommandPool(m_LogicalDevice, m_CommandPool, m_QueueFamilyIndices.graphicsFamily.value());
	}

	GraphicsDevice::~GraphicsDevice() {
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
		vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
	}

	bool GraphicsDevice::CreateSwapChain(Window& window, SwapChain& swapChain) {
		Engine::Graphics::CreateSwapChain(m_PhysicalDevice, m_LogicalDevice, m_Surface, swapChain, window.GetFramebufferSize());

		return true;
	}

	void GraphicsDevice::WaitIdle() {
		Engine::Graphics::WaitIdle(m_LogicalDevice);
	}

	void GraphicsDevice::CreateFramesResources() {
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			{
				VkFenceCreateInfo fenceCreateInfo = {};
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

				VkResult result = vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &frameFences[i]);
				assert(result == VK_SUCCESS);
			}

			{
				VkCommandBufferAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = m_CommandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = 1;

				VkResult result = vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffers[i]);
				assert(result == VK_SUCCESS);
			}
		}
	}

	VkCommandBuffer* GraphicsDevice::BeginFrame(SwapChain& swapChain) {
		VkResult result;

		vkWaitForFences(m_LogicalDevice, 1, &frameFences[currentFrame], VK_TRUE, UINT64_MAX);
		
		VkResult result = vkAcquireNextImageKHR(
			m_LogicalDevice, 
			swapChain.swapChain, 
			UINT64_MAX,
			swapChain.imageAvailableSemaphores[currentFrame],
			VK_NULL_HANDLE, 
			&swapChain.imageIndex
		);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return nullptr;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(m_LogicalDevice, 1, &frameFences[currentFrame]);

		Engine::Graphics::BeginCommandBuffer(m_LogicalDevice, m_CommandPool, commandBuffers[currentFrame]);

		return &commandBuffers[currentFrame];
	}

	void GraphicsDevice::EndFrame(const VkDevice& logicalDevice, const VkCommandBuffer& commandBuffer, const SwapChain& swapChain) {

		VkResult result = vkEndCommandBuffer(commandBuffer);
		assert(result == VK_SUCCESS);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		std::vector<VkCommandBuffer> cmdBuffers = { commandBuffer };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &swapChain.imageAvailableSemaphores[currentFrame];

		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submitInfo.pCommandBuffers = cmdBuffers.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &swapChain.renderFinishedSemaphores[currentFrame];

		result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, frameFences[currentFrame]);
		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::BeginRenderPass(const VkRenderPass& renderPass, VkCommandBuffer& commandBuffer, const VkFramebuffer& framebuffer, const VkExtent2D renderArea) {
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffer;
		renderPassBeginInfo.renderArea.offset = {0, 0};
		renderPassBeginInfo.renderArea.extent = renderArea;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void GraphicsDevice::EndRenderPass(VkCommandBuffer& commandBuffer) {
		vkCmdEndRenderPass(commandBuffer);
	}

	void GraphicsDevice::BindViewport(const Viewport& viewport, VkCommandBuffer& commandBuffer) {

		VkViewport vp = {};
		vp.x = 0.0f;
		vp.y = 0.0f;
		vp.width = viewport.width;
		vp.height = viewport.height;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &vp);
	}

	void GraphicsDevice::BindScissor(const Rect& rect, VkCommandBuffer& commandBuffer) {

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent.width = abs(rect.right - rect.left);
		scissor.extent.height = abs(rect.top - rect.bottom);

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void GraphicsDevice::PresentFrame(const SwapChain& swapChain) {
		VkResult result;

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		//presentInfo.pWaitSemaphores = m_RenderFinishedSemaphores->GetHandle(currentFrame);
		presentInfo.pWaitSemaphores = &swapChain.renderFinishedSemaphores[currentFrame];
		
		VkSwapchainKHR swapChains[] = { swapChain.swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &swapChain.imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
			return;
		} else {
			assert(0);
		}

		// using modulo operator to ensure that the frame index loops around after every FRAMES_IN_FLIGHT enqueued frames
		currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
	}

	void GraphicsDevice::CreateFramebuffer(const VkRenderPass& renderPass, std::vector<VkImageView&> attachmentViews, VkExtent2D& framebufferExtent) {
		std::vector<VkImageView> attachments(attachmentViews.size());

		for (int i = 0; i < attachmentViews.size(); i++) {
			attachments[i] = attachmentViews[i];
		}

		/*
		std::array<VkImageView, 3> attachments = { 
			m_RenderTarget->ImageView,
			m_DepthBuffer->GetDepthBufferImageView(),
			swapChain.swapChainImageViews[i]
		};
		*/

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = framebufferExtent.width;
		framebufferInfo.height = framebufferExtent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &framebuffer);
		assert(result == VK_SUCCESS);
	}

	GraphicsDevice& GraphicsDevice::CreateImage(GPUImage& image, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType) {
		image.Width = width;
		image.Height = height;
		image.MipLevels = mipLevels;
		image.Format = format;
		image.MsaaSamples = m_MsaaSamples;
		image.ImageType = imageType;

		Engine::Graphics::CreateImage(m_LogicalDevice, image);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::RecreateImage(GPUImage& image) {
		Engine::Graphics::CreateImage(m_LogicalDevice, image);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::CreateImageView(GPUImage& image, const VkImageViewType viewType, const VkImageAspectFlags aspectFlags, const uint32_t layerCount) {
		image.ViewType = viewType;
		image.AspectFlags = aspectFlags;
		image.LayerCount = layerCount;

		Engine::Graphics::CreateImageView(m_LogicalDevice, image);
		
		return *this;
	}

	GraphicsDevice& GraphicsDevice::RecreateImageView(GPUImage& image) {
		Engine::Graphics::CreateImageView(m_LogicalDevice, image);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::AllocateMemory(GPUImage& image, VkMemoryPropertyFlagBits memoryProperty) {
		image.MemoryProperty = memoryProperty;
		Engine::Graphics::AllocateMemory(m_PhysicalDevice, m_LogicalDevice, image);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::TransitionImageLayout(GPUImage& image, VkImageLayout newLayout) {
		Engine::Graphics::TransitionImageLayout(m_LogicalDevice, m_CommandPool, m_GraphicsQueue, image, newLayout);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::GenerateMipMaps(GPUImage& image) {
		Engine::Graphics::GenerateImageMipMaps(m_PhysicalDevice, m_LogicalDevice, m_CommandPool, m_GraphicsQueue, image);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::CreateImageSampler(GPUImage& image, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT) {
		Engine::Graphics::CreateImageSampler(m_PhysicalDevice, m_LogicalDevice, image, addressMode);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::ResizeImage(GPUImage& image, uint32_t width, uint32_t height) {
		assert(image.ImageView != VK_NULL_HANDLE && "Image view must be created first!");
		assert(image.Image != VK_NULL_HANDLE && "Image must be created first!");

		Engine::Graphics::DestroyImage(m_LogicalDevice, image);

		image.Width = width;
		image.Height = height;

		RecreateImage(image);
		RecreateImageView(image);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::CopyBufferToImage(GPUImage& image, GPUBuffer& srcBuffer) {
		Engine::Graphics::CopyBufferToImage(m_LogicalDevice, m_CommandPool, m_GraphicsQueue, image, srcBuffer.Handle);

		return *this;
	}

	template <class T>
	GraphicsDevice& GraphicsDevice::UploadDataToImage(GPUImage& image, const T* data, const size_t dataSize) {
		// TODO: Implement after reworking buffers
		return *this;
	}

	GraphicsDevice& GraphicsDevice::DestroyImage(GPUImage& image) {
		vkDestroySampler(m_LogicalDevice, image.ImageSampler, nullptr);
		Engine::Graphics::DestroyImage(m_LogicalDevice, image);

		return *this;
	}

	void GraphicsDevice::CreateDepthBuffer(GPUImage& depthBuffer, uint32_t width, uint32_t height) {
		depthBuffer.Tiling = VK_IMAGE_TILING_OPTIMAL;
		depthBuffer.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthBuffer.MipLevels = 1;

		CreateImage(depthBuffer, width, height, 1, Engine::Graphics::FindDepthFormat(m_PhysicalDevice), VK_IMAGE_TYPE_2D)
			.AllocateMemory(depthBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.CreateImageView(depthBuffer, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		// usage example
		//CreateDepthBuffer(depthBuffer, swapChain.extent.width, swapChain.extent.height);
	}

	void GraphicsDevice::CreateRenderTarget(GPUImage& renderTarget, uint32_t width, uint32_t height, VkFormat imageFormat) {
		renderTarget.Tiling = VK_IMAGE_TILING_OPTIMAL;
		renderTarget.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		renderTarget.MipLevels = 1;

		CreateImage(renderTarget, width, height, 1, imageFormat, VK_IMAGE_TYPE_2D)
			.AllocateMemory(renderTarget, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.CreateImageView(renderTarget, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		// usage example
		//CreateRenderTarget(renderTarget, swapChain.extent.width, swapChain.extent.height, swapChain.imageFormat);
	}

	GraphicsDevice& GraphicsDevice::CreateTexture2D(GPUImage& texture, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat imageFormat) {
		texture.Tiling = VK_IMAGE_TILING_OPTIMAL;
		texture.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		texture.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		texture.Format = imageFormat;

		CreateImage(texture, width, height, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D);
		return *this;

		/* Usage example.
		CreateTexture2D(texture, width, height, mipLevels, VK_FORMAT_R8G8B8A8_SRGB)
			.AllocateMemory(texture, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.CreateImageView(texture, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1)
			.TransitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			.CopyBufferToImage(texture, transferBuffer)
			.GenerateMipMaps()
			.CreateImageSampler(texture, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		*/
	}

	GraphicsDevice& GraphicsDevice::CreateCubeTexture(GPUImage& texture, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat imageFormat) {
		texture.Tiling = VK_IMAGE_TILING_OPTIMAL;
		texture.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		texture.AspectFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		CreateImage(texture, width, height, mipLevels, imageFormat, VK_IMAGE_TYPE_2D);

		/* Usage example.
		CreateCubeTexture(texture, width, height, mipLevels, imageFormat)
			.AllocateMemory(texture, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.CreateImageView(texture, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 6)
			.TransitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			.UploadDataToImage(texture, nullptr, 1)
			.TransitionImageLayout(texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			.CreateImageSampler(texture, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		*/
	}

	void GraphicsDevice::CreateBuffer(GPUBuffer& buffer, size_t bufferSize, VkBufferUsageFlags usage) {
		buffer.BufferSize = bufferSize;
		buffer.Usage = usage;

		Engine::Graphics::CreateBuffer(m_LogicalDevice, buffer);
	}

	GraphicsDevice& GraphicsDevice::AllocateMemory(GPUBuffer& buffer, VkMemoryPropertyFlagBits memoryProperty) {
		buffer.MemoryProperty = memoryProperty;

		Engine::Graphics::AllocateMemory(m_PhysicalDevice, m_LogicalDevice, buffer);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::CopyBuffer(GPUBuffer& srcBuffer, GPUBuffer& dstBuffer, VkDeviceSize size, size_t srcOffset, size_t dstOffset) {
		Engine::Graphics::CopyBuffer(m_LogicalDevice, m_CommandPool, m_GraphicsQueue, srcBuffer.Handle, dstBuffer.Handle, size, srcOffset, dstOffset);
		return *this;
	}

	GraphicsDevice& GraphicsDevice::AddBufferChunk(GPUBuffer& buffer, GPUBuffer::BufferChunk newChunk) {
		buffer.Chunks.push_back(newChunk);

		return *this;
	}

	GraphicsDevice& GraphicsDevice::UpdateBuffer(GPUBuffer& buffer, VkDeviceSize offset, void* data, VkDeviceSize dataSize) {
		vkMapMemory(m_LogicalDevice, buffer.Memory, offset, dataSize, 0, &buffer.MemoryMapped);
		memcpy(buffer.MemoryMapped, data, dataSize);
		vkUnmapMemory(m_LogicalDevice, buffer.Memory);

		return *this;
	}
}