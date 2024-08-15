#include "GraphicsDevice.h"

#include "UI.h"
#include <string>
#include <fstream>

namespace Engine::Graphics {
	VkPhysicalDevice GraphicsDevice::CreatePhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface) {

		assert(instance != VK_NULL_HANDLE);
		assert(surface != VK_NULL_HANDLE);

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	
		std::vector<VkPhysicalDevice> physicalDevicesAvailable;

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		assert(deviceCount != 0 && "A GPU with Vulkan support is required!");

		physicalDevicesAvailable.resize(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevicesAvailable.data());

		std::cout << "Available Devices:" << '\n';

		for (const auto& device : physicalDevicesAvailable) {
			VkPhysicalDeviceFeatures device_features;
			VkPhysicalDeviceProperties device_properties;

			vkGetPhysicalDeviceFeatures(device, &device_features);
			vkGetPhysicalDeviceProperties(device, &device_properties);

			std::cout << '\t' << device_properties.deviceName << '\n';
		}

		for (auto& device : physicalDevicesAvailable) {
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

	QueueFamilyIndices GraphicsDevice::FindQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
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

	bool GraphicsDevice::isDeviceSuitable(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
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

	void GraphicsDevice::CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface) {
		assert(instance != VK_NULL_HANDLE && "Instance must not be VK_NULL_HANDLE");
		assert(surface == VK_NULL_HANDLE && "Surface must be VK_NULL_HANDLE");

		VkResult result = glfwCreateWindowSurface(instance, &window, nullptr, &surface);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::CreateInstance(VkInstance& instance) {

		assert(instance == VK_NULL_HANDLE && "Instance must be VK_NULL_HANDLE!");

		if (c_EnableValidationLayers && !CheckValidationLayerSupport()) {
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

		auto glfwExtensions = GetRequiredExtensions();

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

		CheckRequiredExtensions(static_cast<uint32_t>(glfwExtensions.size()), glfwExtensions.data(),
			extensions);
	}

	bool GraphicsDevice::CheckValidationLayerSupport() {
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

	void GraphicsDevice::CheckRequiredExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions, 
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

	std::vector<const char*> GraphicsDevice::GetRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (c_EnableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void GraphicsDevice::CreateLogicalDevice(QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice) {
		assert(logicalDevice == VK_NULL_HANDLE && "Logical device must be VK_NULL_HANDLE");
		assert(physicalDevice != VK_NULL_HANDLE && "Physical device must not be VK_NULL_HANDLE");

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

	void GraphicsDevice::CreateQueue(VkDevice& logicalDevice, uint32_t queueFamilyIndex, VkQueue& queue) {
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &queue);
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

	SwapChainSupportDetails GraphicsDevice::QuerySwapChainSupportDetails(VkPhysicalDevice& device, VkSurfaceKHR& surface) {
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

	bool GraphicsDevice::CreateSwapChain(Window& window, SwapChain& swapChain) {
		CreateSwapChainInternal(m_PhysicalDevice, m_LogicalDevice, m_Surface, swapChain, window.GetFramebufferSize());
		CreateSwapChainImageViews(m_LogicalDevice, swapChain);
		CreateSwapChainSemaphores(m_LogicalDevice, swapChain);

		return true;
	}

	void GraphicsDevice::CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent) {
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

	void GraphicsDevice::CreateSwapChainImageViews(VkDevice& logicalDevice, SwapChain& swapChain) {
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

	void GraphicsDevice::CreateSwapChainSemaphores(VkDevice& logicalDevice, SwapChain& swapChain) {

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

	void GraphicsDevice::CreateCommandPool(VkCommandPool& commandPool, uint32_t queueFamilyIndex) {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndex;

		VkResult result = vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &commandPool);
		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::CreateCommandBuffer(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer) {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkResult result = vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::BeginCommandBuffer(VkCommandBuffer& commandBuffer) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::EndCommandBuffer(VkCommandBuffer& commandBuffer) {
		VkResult result = vkEndCommandBuffer(commandBuffer);
		assert(result == VK_SUCCESS);
	}

	VkCommandBuffer GraphicsDevice::BeginSingleTimeCommandBuffer(VkCommandPool& commandPool) {
		assert(commandPool != VK_NULL_HANDLE);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = {};
		vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		assert(result == VK_SUCCESS);

		return commandBuffer;
	}

	void GraphicsDevice::EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer, VkCommandPool& commandPool) {

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_GraphicsQueue, 1, &info, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_GraphicsQueue);

		vkFreeCommandBuffers(m_LogicalDevice, commandPool, 1, &commandBuffer);
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

	void GraphicsDevice::CreateImage(GPUImage& image) {
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = image.Description.ImageType;
		imageCreateInfo.extent.width = image.Description.Width;
		imageCreateInfo.extent.height = image.Description.Height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = image.Description.MipLevels;
		imageCreateInfo.arrayLayers = (uint32_t)(image.Description.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? 6 : 1);

		if (image.Description.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
			imageCreateInfo.flags = image.Description.AspectFlags;
		}

		imageCreateInfo.format = image.Description.Format;
		imageCreateInfo.tiling = image.Description.Tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = image.Description.Usage;
		imageCreateInfo.samples = image.Description.MsaaSamples;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateImage(m_LogicalDevice, &imageCreateInfo, nullptr, &image.Image);

		assert(result == VK_SUCCESS);

		AllocateMemory(image, image.Description.MemoryProperty);
	}

	VkFormat GraphicsDevice::FindDepthFormat(VkPhysicalDevice& physicalDevice) {
		return FindSupportedFormat(physicalDevice,
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat GraphicsDevice::FindSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
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

	GraphicsDevice::GraphicsDevice(Window& window) {
		CreateInstance(m_VulkanInstance);
		CreateSurface(m_VulkanInstance, *window.GetHandle(), m_Surface);
		m_PhysicalDevice = CreatePhysicalDevice(m_VulkanInstance, m_Surface);

		m_PhysicalDeviceProperties = GetDeviceProperties(m_PhysicalDevice);
		m_MsaaSamples = GetMaxSampleCount(m_PhysicalDeviceProperties);

		std::cout << "Selected device: " << m_PhysicalDeviceProperties.deviceName << '\n';
		std::cout << "MSAA Samples: " << m_MsaaSamples << '\n';

		m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, m_Surface);

		CreateLogicalDevice(m_QueueFamilyIndices, m_PhysicalDevice, m_LogicalDevice);
		CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.graphicsFamily.value(), m_GraphicsQueue);
		CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.presentFamily.value(), m_PresentQueue);
		CreateQueue(m_LogicalDevice, m_QueueFamilyIndices.graphicsAndComputeFamily.value(), m_ComputeQueue);

		CreateDebugMessenger(m_VulkanInstance, m_DebugMessenger);
		CreateCommandPool(m_CommandPool, m_QueueFamilyIndices.graphicsFamily.value());
		CreateFramesResources();

		CreateDefaultRenderPass(defaultRenderPass);
	}

	GraphicsDevice::~GraphicsDevice() {
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			DestroyCommandBuffer(m_CommandBuffers[i]);
			vkDestroyFence(m_LogicalDevice, m_FrameFences[i], nullptr);
		}

		vkDestroyRenderPass(m_LogicalDevice, defaultRenderPass, nullptr);
		vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
		vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
	}

	void GraphicsDevice::RecreateSwapChain(Window& window, SwapChain& swapChain) {
		VkExtent2D currentExtent = window.GetFramebufferSize();

		while (currentExtent.width == 0 || currentExtent.height == 0) {
			currentExtent = window.GetFramebufferSize();
			window.WaitEvents();
		}

		WaitIdle();

		DestroySwapChain(swapChain);
		CreateSwapChainInternal(m_PhysicalDevice, m_LogicalDevice, m_Surface, swapChain, window.GetFramebufferSize());
		CreateSwapChainImageViews(m_LogicalDevice, swapChain);
		CreateSwapChainSemaphores(m_LogicalDevice, swapChain);
	}

	void GraphicsDevice::DestroySwapChain(SwapChain& swapChain) {
		for (auto imageView : swapChain.swapChainImageViews) {
			vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
		}

		for (auto semaphore : swapChain.imageAvailableSemaphores) {
			vkDestroySemaphore(m_LogicalDevice, semaphore, nullptr);
		}
		
		for (auto semaphore : swapChain.renderFinishedSemaphores) {
			vkDestroySemaphore(m_LogicalDevice, semaphore, nullptr);
		}

		vkDestroySwapchainKHR(m_LogicalDevice, swapChain.swapChain, nullptr);
	}

	void GraphicsDevice::WaitIdle() {
		vkDeviceWaitIdle(m_LogicalDevice);
	}

	void GraphicsDevice::CreateFramesResources() {
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			{
				VkFenceCreateInfo fenceCreateInfo = {};
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				VkResult result = vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &m_FrameFences[i]);
				assert(result == VK_SUCCESS);
			}

			CreateCommandBuffer(m_CommandPool, m_CommandBuffers[i]);
		}
	}

	void GraphicsDevice::DestroyCommandBuffer(VkCommandBuffer& commandBuffer) {
		vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);
	}

	void GraphicsDevice::RecreateCommandBuffers() {
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			DestroyCommandBuffer(m_CommandBuffers[i]);
			CreateCommandBuffer(m_CommandPool, m_CommandBuffers[i]);
		}
	}

	VkCommandBuffer* GraphicsDevice::BeginFrame(SwapChain& swapChain) {

		vkWaitForFences(m_LogicalDevice, 1, &m_FrameFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
		
		VkResult result = vkAcquireNextImageKHR(
			m_LogicalDevice, 
			swapChain.swapChain, 
			UINT64_MAX,
			swapChain.imageAvailableSemaphores[m_CurrentFrame],
			VK_NULL_HANDLE, 
			&swapChain.imageIndex
		);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return nullptr;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(m_LogicalDevice, 1, &m_FrameFences[m_CurrentFrame]);

		BeginCommandBuffer(m_CommandBuffers[m_CurrentFrame]);

		return &m_CommandBuffers[m_CurrentFrame];
	}

	void GraphicsDevice::EndFrame(const VkCommandBuffer& commandBuffer, const SwapChain& swapChain) {

		VkResult result = vkEndCommandBuffer(commandBuffer);
		assert(result == VK_SUCCESS);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		std::vector<VkCommandBuffer> cmdBuffers = { commandBuffer };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &swapChain.imageAvailableSemaphores[m_CurrentFrame];

		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submitInfo.pCommandBuffers = cmdBuffers.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &swapChain.renderFinishedSemaphores[m_CurrentFrame];

		result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_FrameFences[m_CurrentFrame]);
		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::BeginRenderPass(const VkRenderPass& renderPass, VkCommandBuffer& commandBuffer, const VkExtent2D renderArea, uint32_t imageIndex, const VkFramebuffer& framebuffer) {
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

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(renderArea.width);
		viewport.height = static_cast<float>(renderArea.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = renderArea;

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
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

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &swapChain.renderFinishedSemaphores[m_CurrentFrame];
		
		VkSwapchainKHR swapChains[] = { swapChain.swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &swapChain.imageIndex;
		presentInfo.pResults = nullptr;

		VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			m_CurrentFrame = (m_CurrentFrame + 1) % FRAMES_IN_FLIGHT;
			return;
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		// using modulo operator to ensure that the frame index loops around after every FRAMES_IN_FLIGHT enqueued frames
		m_CurrentFrame = (m_CurrentFrame + 1) % FRAMES_IN_FLIGHT;
	}

	void GraphicsDevice::CreateFramebuffer(const VkRenderPass& renderPass, const std::vector<VkImageView>& attachmentViews, const VkExtent2D extent, VkFramebuffer& framebuffer) {

		std::vector<VkImageView> attachments;

		for (int i = 0; i < attachmentViews.size(); i++) {
			attachments.push_back(attachmentViews[i]);
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &framebuffer);
		assert(result == VK_SUCCESS);

	}

	void GraphicsDevice::CreateImageView(GPUImage& image) {
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = image.Image;
		viewCreateInfo.viewType = image.Description.ViewType;
		viewCreateInfo.format = image.Description.Format;
		viewCreateInfo.subresourceRange.aspectMask = (image.Description.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? VK_IMAGE_ASPECT_COLOR_BIT : image.Description.AspectFlags);
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = image.Description.MipLevels;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = image.Description.LayerCount;

		VkResult result = vkCreateImageView(m_LogicalDevice, &viewCreateInfo, nullptr, &image.ImageView);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::AllocateMemory(GPUImage& image, VkMemoryPropertyFlagBits memoryProperty) {
		image.Description.MemoryProperty = memoryProperty;

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_LogicalDevice, image.Image, &memRequirements);
		
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, memRequirements.memoryTypeBits, image.Description.MemoryProperty);

		VkResult result = vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &image.Memory);

		assert(result == VK_SUCCESS);

		vkBindImageMemory(m_LogicalDevice, image.Image, image.Memory, 0);
	}

	void GraphicsDevice::TransitionImageLayout(GPUImage& image, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(m_CommandPool);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = image.ImageLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image.Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = image.Description.MipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = image.Description.LayerCount;

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

		vkCmdPipelineBarrier(commandBuffer, sourceStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndSingleTimeCommandBuffer(commandBuffer, m_CommandPool);

		image.ImageLayout = newLayout;
	}

	void GraphicsDevice::GenerateMipMaps(GPUImage& image) {

		assert(image.Image != VK_NULL_HANDLE);

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, image.Description.Format, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			throw std::runtime_error("Texture image format does not support linear blitting!");
		}

		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(m_CommandPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image.Image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = image.Description.LayerCount;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = image.Description.Width;
		int32_t mipHeight = image.Description.Height;

		for (uint32_t i = 1; i < image.Description.MipLevels; i++) {
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

		barrier.subresourceRange.baseMipLevel = image.Description.MipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);		

		EndSingleTimeCommandBuffer(commandBuffer, m_CommandPool);
		
		image.ImageLayout = barrier.newLayout;
	}

	void GraphicsDevice::CreateImageSampler(GPUImage& image) {
		// TODO: add parameters as variables
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = image.Description.AddressMode;
		samplerInfo.addressModeV = image.Description.AddressMode;
		samplerInfo.addressModeW = image.Description.AddressMode;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
	
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(image.Description.MipLevels);

		VkResult result = vkCreateSampler(m_LogicalDevice, &samplerInfo, nullptr, &image.ImageSampler);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::ResizeImage(GPUImage& image, uint32_t width, uint32_t height) {
		assert(image.ImageView != VK_NULL_HANDLE && "Image view must be created first!");
		assert(image.Image != VK_NULL_HANDLE && "Image must be created first!");

		DestroyImage(image);

		image.Description.Width = width;
		image.Description.Height = height;

		CreateImage(image);
		CreateImageView(image);
	}

	void GraphicsDevice::CopyBufferToImage(GPUImage& image, GPUBuffer& srcBuffer) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(m_CommandPool);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = image.Description.MipLevels;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { image.Description.Width, image.Description.Height, 1 }; 

		vkCmdCopyBufferToImage(commandBuffer, srcBuffer.Handle, image.Image, image.ImageLayout, 1, &region);

		EndSingleTimeCommandBuffer(commandBuffer, m_CommandPool);
	}

	template <class T>
	void GraphicsDevice::UploadDataToImage(GPUImage& dstImage, const T* data, const size_t dataSize) {
		assert(dataSize != 0);

		BufferDescription stagingDesc = {};
		stagingDesc.BufferSize = dataSize;
		stagingDesc.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingDesc.MemoryProperty = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		GPUBuffer stagingBuffer = {};
		stagingBuffer.Description = stagingDesc;

		CreateBuffer(stagingDesc, stagingBuffer, dataSize);
		vkMapMemory(m_LogicalDevice, stagingBuffer.Memory, 0, dataSize, 0, &stagingBuffer.MemoryMapped);
		memcpy(stagingBuffer.MemoryMapped, data, dataSize);
		vkUnmapMemory(m_LogicalDevice, stagingBuffer.Memory);

		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(m_CommandPool);

		const VkBufferImageCopy region = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = dstImage.Description.LayerCount 
			},
			.imageOffset = {
				.x = 0,
				.y = 0,
				.z = 0
			},
			.imageExtent = {
				.width = dstImage.Description.Width,
				.height = dstImage.Description.Height,
				.depth = 1
			}
		};

		vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.Handle, dstImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		EndSingleTimeCommandBuffer(commandBuffer, m_CommandPool);
		DestroyBuffer(stagingBuffer);
	}

	void GraphicsDevice::DestroyImage(GPUImage& image) {
		vkDestroySampler(m_LogicalDevice, image.ImageSampler, nullptr);
		vkDestroyImageView(m_LogicalDevice, image.ImageView, nullptr);
		vkDestroyImage(m_LogicalDevice, image.Image, nullptr);
		vkFreeMemory(m_LogicalDevice, image.Memory, nullptr);
	}

	void GraphicsDevice::CreateDepthBuffer(GPUImage& depthBuffer, uint32_t width, uint32_t height) {

		ImageDescription depthDesc = {};
		depthDesc.Width = width;
		depthDesc.Height = height;
		depthDesc.MipLevels = 1;
		depthDesc.MsaaSamples = m_MsaaSamples;
		depthDesc.Tiling = VK_IMAGE_TILING_OPTIMAL;
		depthDesc.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthDesc.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depthDesc.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthDesc.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		depthDesc.LayerCount = 1;
		depthDesc.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		depthDesc.Format = FindDepthFormat(m_PhysicalDevice);
		depthDesc.ImageType = VK_IMAGE_TYPE_2D;

		depthBuffer.Description = depthDesc;

		CreateImage(depthBuffer);
		CreateImageView(depthBuffer);
	}

	void GraphicsDevice::CreateRenderTarget(GPUImage& renderTarget, uint32_t width, uint32_t height, VkFormat format) {

		ImageDescription renderTargetDesc = {};
		renderTargetDesc.Width = width;
		renderTargetDesc.Height = height;
		renderTargetDesc.MipLevels = 1;
		renderTargetDesc.MsaaSamples = m_MsaaSamples;
		renderTargetDesc.Tiling = VK_IMAGE_TILING_OPTIMAL;
		renderTargetDesc.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		renderTargetDesc.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		renderTargetDesc.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		renderTargetDesc.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		renderTargetDesc.LayerCount = 1;
		renderTargetDesc.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		renderTargetDesc.Format = format;
		renderTargetDesc.ImageType = VK_IMAGE_TYPE_2D;

		renderTarget.Description = renderTargetDesc;

		CreateImage(renderTarget);
		CreateImageView(renderTarget);
	}

	void GraphicsDevice::AllocateMemory(GPUBuffer& buffer, VkMemoryPropertyFlagBits memoryProperty) {
		buffer.Description.MemoryProperty = memoryProperty;

		VkMemoryRequirements memRequirements = GetMemoryRequirements(m_LogicalDevice, buffer.Handle);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size == 0 ? 256 : memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, memRequirements.memoryTypeBits, buffer.Description.MemoryProperty);

		VkResult result = vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &buffer.Memory);

		assert(result == VK_SUCCESS);

		vkBindBufferMemory(m_LogicalDevice, buffer.Handle, buffer.Memory, 0);
	}

	void GraphicsDevice::CopyBuffer(GPUBuffer& srcBuffer, GPUBuffer& dstBuffer, VkDeviceSize size, size_t srcOffset, size_t dstOffset) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommandBuffer(m_CommandPool);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer.Handle, dstBuffer.Handle, 1, &copyRegion);

		EndSingleTimeCommandBuffer(commandBuffer, m_CommandPool);
	}

	void GraphicsDevice::AddBufferChunk(GPUBuffer& buffer, BufferDescription::BufferChunk newChunk) {
		buffer.Description.Chunks.push_back(newChunk);
	}

	void GraphicsDevice::DestroyBuffer(GPUBuffer& buffer) {
		vkDestroyBuffer(m_LogicalDevice, buffer.Handle, nullptr);
		vkFreeMemory(m_LogicalDevice, buffer.Memory, nullptr);
	}

	template <class T>
	void GraphicsDevice::CopyDataFromStaging(GPUBuffer& dstBuffer, T* data, size_t dataSize, size_t offset) {
		BufferDescription stagingDesc = {};
		stagingDesc.BufferSize = dataSize;
		stagingDesc.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingDesc.MemoryProperty = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		GPUBuffer stagingBuffer = {};
		stagingBuffer.Description = stagingDesc;

		CreateBuffer(stagingDesc, stagingBuffer, dataSize);

		vkMapMemory(m_LogicalDevice, stagingBuffer.Memory, 0, dataSize, 0, &stagingBuffer.MemoryMapped);
		memcpy(stagingBuffer.MemoryMapped, data, dataSize);
		vkUnmapMemory(m_LogicalDevice, stagingBuffer.Memory);

		CopyBuffer(stagingBuffer, dstBuffer, dataSize, 0, offset);
		DestroyBuffer(stagingBuffer);
	}

	void GraphicsDevice::CreateBuffer(BufferDescription& desc, GPUBuffer& buffer, size_t bufferSize) {
		desc.BufferSize = bufferSize;
		buffer.Description = desc;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = buffer.Description.BufferSize;
		bufferInfo.usage = buffer.Description.Usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &buffer.Handle);

		assert(result == VK_SUCCESS);

		AllocateMemory(buffer, buffer.Description.MemoryProperty);
	}

	// TODO: implement dynamic allocation/update/retrieval
	void GraphicsDevice::WriteBuffer(GPUBuffer& buffer, const void* data, size_t size, size_t offset) {
		if (data == nullptr)
			return;

		CopyDataFromStaging(buffer, data, std::min(buffer.Description.BufferSize, size), offset);
	}

	void GraphicsDevice::UpdateBuffer(GPUBuffer& buffer, VkDeviceSize offset, void* data, size_t dataSize) {
		vkMapMemory(m_LogicalDevice, buffer.Memory, offset, dataSize, 0, &buffer.MemoryMapped);
		memcpy(buffer.MemoryMapped, data, dataSize);
		vkUnmapMemory(m_LogicalDevice, buffer.Memory);
	}

	void GraphicsDevice::CreateTexture(ImageDescription& desc, Texture& texture, Texture::TextureType textureType, void* initialData, size_t dataSize) {
		texture.Description = desc;

		CreateImage(texture);
		CreateImageView(texture);
		TransitionImageLayout(texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		UploadDataToImage(texture, initialData, dataSize);
		GenerateMipMaps(texture);
		CreateImageSampler(texture);
	}

	void GraphicsDevice::CreateDefaultRenderPass(VkRenderPass& renderPass) {
		SwapChainSupportDetails swapChainSupportDetails = QuerySwapChainSupportDetails(m_PhysicalDevice, m_Surface);
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = surfaceFormat.format;
		colorAttachment.samples = m_MsaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = FindDepthFormat(m_PhysicalDevice);
		depthAttachment.samples = m_MsaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = surfaceFormat.format;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentReference{};
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentReference;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;

		std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		std::vector<VkSubpassDescription> subpasses = { subpass };
		std::vector<VkSubpassDependency> dependencies = { dependency };

		CreateRenderPass(renderPass, attachments, subpasses, dependencies);
	}

	void GraphicsDevice::CreateRenderPass(VkRenderPass& renderPass, std::vector<VkAttachmentDescription> attachments, std::vector<VkSubpassDescription> subpass, std::vector<VkSubpassDependency> dependencies) {
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(subpass.size());
		renderPassInfo.pSubpasses = subpass.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VkResult result = vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &renderPass);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::DestroyRenderPass(VkRenderPass& renderPass) {
		vkDestroyRenderPass(m_LogicalDevice, renderPass, nullptr);
	}

	void GraphicsDevice::RecreateDefaultRenderPass(VkRenderPass& renderPass, SwapChain& swapChain) {
		DestroyRenderPass(renderPass);
		CreateDefaultRenderPass(renderPass);
	}

	void GraphicsDevice::DestroyFramebuffer(std::vector<VkFramebuffer>& framebuffers) {
		for (int i = 0; i < framebuffers.size(); i++) {
			vkDestroyFramebuffer(m_LogicalDevice, framebuffers[i], nullptr);
		}
	}

	void GraphicsDevice::CreateDescriptorPool() {
		VkDescriptorPoolSize poolSizes[2] = {};
		uint32_t count = 0;

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = m_PoolSize;
		count++;
		
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = m_PoolSize;
		count++;

		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = count;
		poolCreateInfo.maxSets = m_PoolSize;
		poolCreateInfo.pPoolSizes = poolSizes;

		VkResult result = vkCreateDescriptorPool(m_LogicalDevice, &poolCreateInfo, nullptr, &m_DescriptorPool);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::DestroyDescriptorPool() {
		vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
	}

	void GraphicsDevice::CreatePipelineLayout(PipelineLayoutDesc desc, VkPipelineLayout& pipelineLayout) {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(desc.SetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = desc.SetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(desc.PushConstantRanges.size());
		pipelineLayoutInfo.pPushConstantRanges = desc.PushConstantRanges.data();

		VkResult result = vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::CreateDescriptorSet(InputLayout& inputLayout, VkDescriptorSet& descriptorSet) {
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		
		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = static_cast<uint32_t>(inputLayout.bindings.size());
		createInfo.pBindings = inputLayout.bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(m_LogicalDevice, &createInfo, nullptr, &layout);
		assert(result == VK_SUCCESS);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		result = vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &descriptorSet);
		assert(result == VK_SUCCESS);
	}
	
	void GraphicsDevice::CreateDescriptorSet(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, VkDescriptorSet& descriptorSet) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = descriptorSetLayouts.data();

		VkResult result = vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &descriptorSet);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::CreateDescriptorSet(VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSet& descriptorSet) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &descriptorSet);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::BindDescriptorSet(
		VkDescriptorSet& descriptorSet, 
		const VkCommandBuffer& commandBuffer, 
		const VkPipelineLayout& pipelineLayout,
		uint32_t set,
		uint32_t setCount
	) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, set, setCount, &descriptorSet, 0, nullptr);
	}

	void GraphicsDevice::WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, 
		const VkBuffer& buffer, const size_t bufferSize, const size_t bufferOffset) {
		
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = buffer;
		bufferInfo.offset = bufferOffset;
		bufferInfo.range = bufferSize == 0 ? 256 : bufferSize;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = binding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = binding.descriptorType;
		descriptorWrite.descriptorCount = binding.descriptorCount;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr);
	}

	void GraphicsDevice::WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, std::vector<Texture>& textures) {

		if (textures.size() == 0)
			return;

		std::vector<VkDescriptorImageInfo> imageInfo;
		VkWriteDescriptorSet descriptorWrite = {};

		for (auto& texture : textures) {
			VkDescriptorImageInfo newImageInfo = {};
			newImageInfo.imageLayout = texture.ImageLayout;
			newImageInfo.imageView = texture.ImageView;
			newImageInfo.sampler = texture.ImageSampler;

			imageInfo.push_back(newImageInfo);
		}

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = binding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = binding.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfo.size());
		descriptorWrite.pImageInfo = imageInfo.data();
		
		vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr); 
	}

	void GraphicsDevice::WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, Texture& texture) {

		VkDescriptorImageInfo newImageInfo = {};
		newImageInfo.imageLayout = texture.ImageLayout;
		newImageInfo.imageView = texture.ImageView;
		newImageInfo.sampler = texture.ImageSampler;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = binding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = binding.descriptorType;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &newImageInfo;
		
		vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr); 
	}

	std::vector<char> GraphicsDevice::ReadFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;

	}

	void GraphicsDevice::LoadShader(VkShaderStageFlagBits shaderStage, Shader& shader, const std::string filename) {
		auto shaderCode = ReadFile(filename);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		VkResult result = vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shader.shaderModule);
		assert(result == VK_SUCCESS);

		shader.shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader.shaderStageInfo.stage = shaderStage;
		shader.shaderStageInfo.module = shader.shaderModule;
		shader.shaderStageInfo.pName = "main";
	}

	void GraphicsDevice::DestroyShader(Shader& shader) {
		vkDestroyShaderModule(m_LogicalDevice, shader.shaderModule, nullptr);
	}

	void GraphicsDevice::CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso) {
		CreatePipelineState(desc, pso, defaultRenderPass);
	}

	void GraphicsDevice::CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso, VkRenderPass& renderPass) {

		if (desc.vertexShader != nullptr) {
			auto bindingDescription = desc.vertexShader->BindingDescription;
			auto attributeDescriptions = desc.vertexShader->AttributeDescriptions;

			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
			pso.pipelineInfo.pVertexInputState = &vertexInputInfo;
		}

		pso.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pso.inputAssembly.topology = desc.topology;
		pso.inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkExtent2D swapChainExtent = desc.pipelineExtent;
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)desc.pipelineExtent.width;
		viewport.height = (float)desc.pipelineExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		pso.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pso.viewportState.viewportCount = 1;
		pso.viewportState.pViewports = &viewport;
		pso.viewportState.scissorCount = 1;
		pso.viewportState.pScissors = &scissor;

		pso.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pso.rasterizer.depthClampEnable = VK_FALSE;
		pso.rasterizer.rasterizerDiscardEnable = VK_FALSE;
		pso.rasterizer.polygonMode = desc.polygonMode;
		pso.rasterizer.lineWidth = desc.lineWidth;
		pso.rasterizer.cullMode = desc.cullMode;
		pso.rasterizer.frontFace = desc.frontFace;
		pso.rasterizer.depthBiasEnable = VK_FALSE;
		pso.rasterizer.depthBiasConstantFactor = 0.0f;
		pso.rasterizer.depthBiasClamp = 0.0f;
		pso.rasterizer.depthBiasSlopeFactor = 0.0f;

		pso.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pso.multisampling.sampleShadingEnable = VK_FALSE;
		pso.multisampling.rasterizationSamples = m_MsaaSamples;
		pso.multisampling.minSampleShading = 1.0f;
		pso.multisampling.pSampleMask = nullptr;
		pso.multisampling.alphaToCoverageEnable = VK_FALSE;
		pso.multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		if (desc.vertexShader != nullptr)
			shaderStages.push_back(desc.vertexShader->shaderStageInfo);
		if (desc.fragmentShader != nullptr)
			shaderStages.push_back(desc.fragmentShader->shaderStageInfo);
		if (desc.computeShader != nullptr)
			shaderStages.push_back(desc.computeShader->shaderStageInfo);

		for (auto stage : shaderStages) {
			stage.pNext = nullptr;
			stage.pSpecializationInfo = nullptr;
		}

		pso.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pso.depthStencil.depthTestEnable = VK_TRUE;
		pso.depthStencil.depthWriteEnable = VK_TRUE;
		pso.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		pso.depthStencil.depthBoundsTestEnable = VK_FALSE;
		pso.depthStencil.minDepthBounds = 0.0f;
		pso.depthStencil.maxDepthBounds = 1.0f;
		pso.depthStencil.stencilTestEnable = VK_FALSE;
		pso.depthStencil.front = {};
		pso.depthStencil.back = {};

		for (auto inputLayout : desc.psoInputLayout) {
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(inputLayout.bindings.size());
			layoutInfo.pBindings = inputLayout.bindings.data();
		
			VkResult result = vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &layout);
			assert(result == VK_SUCCESS);

			pso.layoutBindings.insert(pso.layoutBindings.end(), inputLayout.bindings.begin(), inputLayout.bindings.end());
			pso.pushConstants.insert(pso.pushConstants.end(), inputLayout.pushConstants.begin(), inputLayout.pushConstants.end());

			pso.descriptorSetLayout.push_back(layout);

		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(pso.descriptorSetLayout.size());
		pipelineLayoutInfo.pSetLayouts = pso.descriptorSetLayout.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pso.pushConstants.size());
		pipelineLayoutInfo.pPushConstantRanges = pso.pushConstants.data();

		VkResult result = vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pso.pipelineLayout);
		assert(result == VK_SUCCESS);

		pso.pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pso.pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pso.pipelineInfo.pStages = shaderStages.data();
		pso.pipelineInfo.pInputAssemblyState = &pso.inputAssembly;
		pso.pipelineInfo.pViewportState = &pso.viewportState;
		pso.pipelineInfo.pRasterizationState = &pso.rasterizer;
		pso.pipelineInfo.pMultisampleState = &pso.multisampling;
		pso.pipelineInfo.pDepthStencilState = &pso.depthStencil;
		pso.pipelineInfo.pColorBlendState = &colorBlending;
		pso.pipelineInfo.pDynamicState = &dynamicState;
		pso.pipelineInfo.layout = pso.pipelineLayout;
		pso.pipelineInfo.renderPass = renderPass;		// this should be changed to render pass info in the future
		pso.pipelineInfo.subpass = 0;
		pso.pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		//pipelineInfo.basePipelineIndex = -1;

		result = vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pso.pipelineInfo, nullptr, &pso.pipeline);
		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::DestroyPipeline(PipelineState& pso) {
		vkDestroyPipelineLayout(m_LogicalDevice, pso.pipelineLayout, nullptr);

		for (auto descriptorSetLayout : pso.descriptorSetLayout) {
			vkDestroyDescriptorSetLayout(m_LogicalDevice, descriptorSetLayout, nullptr);
		}

		vkDestroyPipeline(m_LogicalDevice, pso.pipeline, nullptr);
	}
}