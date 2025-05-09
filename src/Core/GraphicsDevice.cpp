#include "GraphicsDevice.h"

#include "UI.h"
#include "BufferManager.h"
#include "RenderTarget.h"

#include "../Utils/Helper.h"

#include <string>
#include <fstream>

namespace Graphics {
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

		CheckRequiredExtensions(static_cast<uint32_t>(glfwExtensions.size()), glfwExtensions.data(), extensions);
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
			VkDeviceQueueCreateInfo queueCreateInfo	= {};
			queueCreateInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex		= queueFamily;
			queueCreateInfo.queueCount				= 1;
			queueCreateInfo.pQueuePriorities		= &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures		= {};
		deviceFeatures.samplerAnisotropy			= VK_TRUE;
		deviceFeatures.fillModeNonSolid				= VK_TRUE;
		deviceFeatures.wideLines					= VK_TRUE;
		deviceFeatures.alphaToOne					= VK_TRUE;
		deviceFeatures.geometryShader				= VK_TRUE;

		VkDeviceCreateInfo createInfo				= {};
		createInfo.sType							= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount				= static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos				= queueCreateInfos.data();

		//createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount											= static_cast<uint32_t>(c_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames											= c_DeviceExtensions.data();

		VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingCreateInfo		= {};
		descriptorIndexingCreateInfo.sType											= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		descriptorIndexingCreateInfo.runtimeDescriptorArray							= VK_TRUE;
		descriptorIndexingCreateInfo.pNext											= nullptr;

		VkPhysicalDeviceFeatures2 deviceFeatures2	= {};
		deviceFeatures2.sType						= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.features					= deviceFeatures;
		deviceFeatures2.pNext						= &descriptorIndexingCreateInfo;

		vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

		createInfo.pNext = &deviceFeatures2;

		if (c_EnableValidationLayers) {
			createInfo.enabledLayerCount	= static_cast<uint32_t>(c_ValidationLayers.size());
			createInfo.ppEnabledLayerNames	= c_ValidationLayers.data();
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

	// TODO: remove window from this function and use only the extent instead 
	bool GraphicsDevice::CreateSwapChain(Window& window, SwapChain& swapChain) {
		CreateSwapChainInternal(m_PhysicalDevice, m_LogicalDevice, m_Surface, swapChain, window.GetFramebufferSize());
		CreateSwapChainImageViews(swapChain);

		uint32_t width = window.GetFramebufferSize().width;
		uint32_t height = window.GetFramebufferSize().height;

		// Create swap chain samplers
		{
			swapChain.ImageSamplers.resize(swapChain.ImageViews.size());
			
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
			samplerInfo.maxLod = static_cast<float>(1);

			for (int i = 0; i < swapChain.ImageViews.size(); i++) {
				VkResult result = vkCreateSampler(m_LogicalDevice, &samplerInfo, nullptr, &swapChain.ImageSamplers[i]);
				
				assert(result == VK_SUCCESS);
			}
		}

		return true;
	}

	void GraphicsDevice::CreateSwapChainRenderTarget() {
		m_SwapChain.RenderTarget = std::make_unique<SwapChainRenderTarget>(m_SwapChain.Extent.width, m_SwapChain.Extent.height);
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
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain.Handle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(logicalDevice, swapChain.Handle, &imageCount, nullptr);
		swapChain.Images.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain.Handle, &imageCount, swapChain.Images.data());
		swapChain.ImageFormat = surfaceFormat.format;
		swapChain.Extent = extent;
	}

	void GraphicsDevice::CreateSwapChainImageViews(SwapChain& swapChain) {

		swapChain.ImageViews.resize(swapChain.Images.size());

		for (size_t i = 0; i < swapChain.Images.size(); i++) {
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChain.Images[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChain.ImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &swapChain.ImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image views");
			}
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
		imageCreateInfo.arrayLayers = (uint32_t)(image.Description.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? 6 : image.Description.LayerCount);

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

	VkFormat GraphicsDevice::FindDepthOnlyFormat() {
		return FindSupportedFormat(m_PhysicalDevice, { VK_FORMAT_D32_SFLOAT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	VkFormat GraphicsDevice::FindSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling,
		VkFormatFeatureFlags features) {

		for (VkFormat format : candidates) {
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
				if ((features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) && HasStencilComponent(format)) {
					return format;
				} else if (!(features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
					return format;
				}
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
				if ((features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) && HasStencilComponent(format)) {
					return format;
				} else if (!(features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
					return format;
				}
			}
		}

		throw std::runtime_error("Failed to find supported format!");
	}

	bool GraphicsDevice::HasStencilComponent(VkFormat format) {
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

		m_BufferManager = std::make_unique<BufferManager>();

		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			CreateFrameResources(m_Frames[i]);
		}

		bool success = CreateSwapChain(window, m_SwapChain);
		assert(success);
	}

	GraphicsDevice::~GraphicsDevice() {
		DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);

		m_BufferManager.reset();

		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			DestroyFrameResources(m_Frames[i]);
		}

		m_SwapChain.RenderTarget.reset();
		DestroySwapChain(m_SwapChain);

		vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
		vkDestroyDevice(m_LogicalDevice, nullptr);
		vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
		vkDestroyInstance(m_VulkanInstance, nullptr);
	}

	void GraphicsDevice::RecreateSwapChain(Window& window) {
		VkExtent2D currentExtent = window.GetFramebufferSize();

		while (currentExtent.width == 0 || currentExtent.height == 0) {
			currentExtent = window.GetFramebufferSize();
			window.WaitEvents();
		}

		WaitIdle();

		DestroySwapChain(m_SwapChain);
		CreateSwapChain(window, m_SwapChain);
		m_SwapChain.RenderTarget->Resize(currentExtent.width, currentExtent.height);
	}

	void GraphicsDevice::DestroySwapChain(SwapChain& swapChain) {
		for (auto imageView : swapChain.ImageViews) {
			vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
		}

		for (auto sampler : swapChain.ImageSamplers) {
			vkDestroySampler(m_LogicalDevice, sampler, nullptr);
		}

		vkDestroySwapchainKHR(m_LogicalDevice, swapChain.Handle, nullptr);
	}

	void GraphicsDevice::WaitIdle() {
		vkDeviceWaitIdle(m_LogicalDevice);
	}

	void GraphicsDevice::CreateFrameResources(Frame& frame) {
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkResult result = vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &frame.renderFence);
		assert(result == VK_SUCCESS);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		result = vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore);
		assert(result == VK_SUCCESS);

		result = vkCreateSemaphore(m_LogicalDevice, &semaphoreCreateInfo, nullptr, &frame.swapChainSemaphore);
		assert(result == VK_SUCCESS);

		CreateCommandPool(frame.commandPool, m_QueueFamilyIndices.graphicsFamily.value());
		CreateCommandBuffer(frame.commandPool, frame.commandBuffer);
	}

	void GraphicsDevice::DestroyFrameResources(Frame& frame) {
		vkDestroyFence(m_LogicalDevice, frame.renderFence, nullptr);
		vkDestroySemaphore(m_LogicalDevice, frame.renderSemaphore, nullptr);
		vkDestroySemaphore(m_LogicalDevice, frame.swapChainSemaphore, nullptr);
		vkFreeCommandBuffers(m_LogicalDevice, frame.commandPool, 1, &frame.commandBuffer);
		vkDestroyCommandPool(m_LogicalDevice, frame.commandPool, nullptr);
	}

	bool GraphicsDevice::BeginFrame(Frame& frame) {

		vkWaitForFences(m_LogicalDevice, 1, &frame.renderFence, VK_TRUE, UINT64_MAX);
		
		VkResult result = vkAcquireNextImageKHR(
			m_LogicalDevice, 
			m_SwapChain.Handle, 
			UINT64_MAX,
			frame.swapChainSemaphore,
			VK_NULL_HANDLE, 
			&m_SwapChain.ImageIndex
		);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return false;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(m_LogicalDevice, 1, &frame.renderFence);

		BeginCommandBuffer(frame.commandBuffer);

		return true;
	}

	void GraphicsDevice::EndFrame(const Frame& frame) {

		VkResult result = vkEndCommandBuffer(frame.commandBuffer);
		assert(result == VK_SUCCESS);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		std::vector<VkCommandBuffer> cmdBuffers = { frame.commandBuffer };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &frame.swapChainSemaphore;

		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submitInfo.pCommandBuffers = cmdBuffers.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &frame.renderSemaphore;

		result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, frame.renderFence);
		assert(result == VK_SUCCESS);
	}

	/*
	void GraphicsDevice::BeginRenderPass(const RenderPass& renderPass, const VkCommandBuffer& commandBuffer) {
		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass.Handle;
		renderPassBeginInfo.framebuffer = renderPass.framebuffers[m_SwapChain.ImageIndex];
		renderPassBeginInfo.renderArea.offset = renderPass.Description.offset;
		renderPassBeginInfo.renderArea.extent = renderPass.Description.extent;
		renderPassBeginInfo.pNext = nullptr;

		if (renderPass.Description.clearValues.size() == 0) {
			std::array<VkClearValue, 3> clearValues = {};
			int clearValuesCount = 0;

			if (renderPass.Description.Flags & eColorAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };
			if (renderPass.Description.Flags & eDepthAttachment)
				clearValues[clearValuesCount++] = { .depthStencil{ 1.0f, 0 } };
			if (renderPass.Description.Flags & eResolveAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };

			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValuesCount);
			renderPassBeginInfo.pClearValues = clearValues.data();
		}
		else {
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(renderPass.Description.clearValues.size());
			renderPassBeginInfo.pClearValues = renderPass.Description.clearValues.data();
		}

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = renderPass.Description.viewport;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = renderPass.Description.scissor;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void GraphicsDevice::EndRenderPass(const VkCommandBuffer& commandBuffer) {
		vkCmdEndRenderPass(commandBuffer);
	}
	*/

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

	void GraphicsDevice::PresentFrame(const Frame& frame) {

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frame.renderSemaphore;
		
		VkSwapchainKHR swapChains[] = { m_SwapChain.Handle };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_SwapChain.ImageIndex;
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

	void GraphicsDevice::CreateFramebuffer(const VkRenderPass& renderPass, const std::vector<VkImageView>& attachmentViews, const VkExtent2D extent, VkFramebuffer& framebuffer, const uint32_t layers) {

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
		framebufferInfo.layers = layers;

		VkResult result = vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &framebuffer);
		assert(result == VK_SUCCESS);

	}

	void GraphicsDevice::CreateImageView(GPUImage& image) {
		VkImageViewCreateInfo viewCreateInfo			= {};
		viewCreateInfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image							= image.Image;
		viewCreateInfo.viewType							= image.Description.ViewType;
		viewCreateInfo.format							= image.Description.Format;
		viewCreateInfo.subresourceRange.aspectMask		= (image.Description.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? VK_IMAGE_ASPECT_COLOR_BIT : image.Description.AspectFlags);
		viewCreateInfo.subresourceRange.baseMipLevel	= 0;
		viewCreateInfo.subresourceRange.levelCount		= image.Description.MipLevels;
		viewCreateInfo.subresourceRange.baseArrayLayer	= 0;
		viewCreateInfo.subresourceRange.layerCount		= image.Description.LayerCount;

		if (image.Description.AspectFlags & VK_IMAGE_ASPECT_STENCIL_BIT && HasStencilComponent(image.Description.Format)) {
			viewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

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

	void GraphicsDevice::TransitionImageLayout(
		const VkImage& image, 
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const VkAccessFlags srcAccessMask,
		const VkAccessFlags dstAccessMask,
		const VkPipelineStageFlags srcPipelineStage,
		const VkPipelineStageFlags dstPipelineStage
	) {
		VkCommandBuffer singleTimeCommandBuffer = BeginSingleTimeCommandBuffer(m_CommandPool);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;

		VkPipelineStageFlags sourceStage = srcPipelineStage;
		VkPipelineStageFlags dstStage = dstPipelineStage;

		vkCmdPipelineBarrier(singleTimeCommandBuffer, sourceStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndSingleTimeCommandBuffer(singleTimeCommandBuffer, m_CommandPool);
	}

	void GraphicsDevice::TransitionImageLayout(GPUImage& image, VkImageLayout oldLayout, VkImageLayout newLayout) {
		image.ImageLayout = oldLayout;

		TransitionImageLayout(image, newLayout);
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
		//barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.aspectMask = image.Description.AspectFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ? VK_IMAGE_ASPECT_COLOR_BIT : image.Description.AspectFlags;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = image.Description.MipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = image.Description.LayerCount;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags dstStage;

		if (image.Description.AspectFlags & VK_IMAGE_ASPECT_STENCIL_BIT && HasStencilComponent(image.Description.Format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		if (image.ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		} 
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage	= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else if (image.ImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dstStage	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
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

		if (HasStencilComponent(image.Description.Format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

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
//		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
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
		stagingDesc.Capacity = dataSize;
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
		if (image.ImageSampler != VK_NULL_HANDLE)
			vkDestroySampler(m_LogicalDevice, image.ImageSampler, nullptr);
		if (image.ImageView != VK_NULL_HANDLE)
			vkDestroyImageView(m_LogicalDevice, image.ImageView, nullptr);
		if (image.Image != VK_NULL_HANDLE)
			vkDestroyImage(m_LogicalDevice, image.Image, nullptr);

		image.ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		vkFreeMemory(m_LogicalDevice, image.Memory, nullptr);
	}

	void GraphicsDevice::CreateDepthBuffer(GPUImage& depthBuffer, const RenderPassDesc& renderPassDesc) {

		// Depth + Stencil

		ImageDescription depthDesc	= {};
		depthDesc.Width				= renderPassDesc.Extent.width;
		depthDesc.Height			= renderPassDesc.Extent.height;
		depthDesc.MipLevels			= 1;
		depthDesc.MsaaSamples		= renderPassDesc.SampleCount;
		depthDesc.Tiling			= VK_IMAGE_TILING_OPTIMAL;
		depthDesc.Usage				= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthDesc.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depthDesc.AspectFlags		= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthDesc.ViewType			= VK_IMAGE_VIEW_TYPE_2D;
		depthDesc.LayerCount		= 1;
		depthDesc.AddressMode		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		depthDesc.Format			= FindDepthFormat(m_PhysicalDevice);
		depthDesc.ImageType			= VK_IMAGE_TYPE_2D;

		depthBuffer.Description = depthDesc;

		CreateImage		(depthBuffer);
		CreateImageView	(depthBuffer);
	}

	void GraphicsDevice::CreateDepthBuffer(GPUImage& depthBuffer, const VkExtent2D& extent, const VkSampleCountFlagBits& samples) {

		ImageDescription depthDesc	= {};
		depthDesc.Width				= extent.width;
		depthDesc.Height			= extent.height;
		depthDesc.MipLevels			= 1;
		depthDesc.MsaaSamples		= samples;
		depthDesc.Tiling			= VK_IMAGE_TILING_OPTIMAL;
		depthDesc.Usage				= static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		depthDesc.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depthDesc.AspectFlags		= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthDesc.ViewType			= VK_IMAGE_VIEW_TYPE_2D;
		depthDesc.LayerCount		= 1;
		depthDesc.AddressMode		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		depthDesc.Format			= FindDepthFormat(m_PhysicalDevice);
		depthDesc.ImageType			= VK_IMAGE_TYPE_2D;

		depthBuffer.Description		= depthDesc;

		CreateImage		(depthBuffer);
		CreateImageView	(depthBuffer);
	}

	void GraphicsDevice::CreateDepthOnlyBuffer(GPUImage& depthBuffer, const VkExtent2D extent, const VkSampleCountFlagBits sampleCount, const uint32_t layers) {

		ImageDescription depthDesc	= {};
		depthDesc.Width				= extent.width;
		depthDesc.Height			= extent.height;
		depthDesc.MipLevels			= 1;
		depthDesc.MsaaSamples		= sampleCount;
		depthDesc.Tiling			= VK_IMAGE_TILING_OPTIMAL;
		depthDesc.Usage				= static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		depthDesc.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		depthDesc.AspectFlags		= VK_IMAGE_ASPECT_DEPTH_BIT;// | VK_IMAGE_ASPECT_STENCIL_BIT; // testing stencil bit
		depthDesc.ViewType			= layers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		depthDesc.LayerCount		= layers;
		depthDesc.AddressMode		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
//		depthDesc.AddressMode		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
//		depthDesc.Format			= FindDepthFormat(m_PhysicalDevice);
//		depthDesc.Format			= FindDepthOnlyFormat();
		depthDesc.Format			= VK_FORMAT_D32_SFLOAT;
		depthDesc.ImageType			= VK_IMAGE_TYPE_2D;

		depthBuffer.Description = depthDesc;

		CreateImage		(depthBuffer);
		CreateImageView	(depthBuffer);
		
		depthBuffer.ImageLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
	}

	void GraphicsDevice::CreateRenderTarget(GPUImage& renderTarget, const VkFormat& format, const VkExtent2D& extent, const VkSampleCountFlagBits& samples) {

		ImageDescription renderTargetDesc	= {};
		renderTargetDesc.Width				= extent.width;
		renderTargetDesc.Height				= extent.height;
		renderTargetDesc.MipLevels			= 1;
		renderTargetDesc.MsaaSamples		= samples;
		renderTargetDesc.Tiling				= VK_IMAGE_TILING_OPTIMAL;
		renderTargetDesc.Usage				= static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		renderTargetDesc.MemoryProperty		= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		renderTargetDesc.AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
		renderTargetDesc.ViewType			= VK_IMAGE_VIEW_TYPE_2D;
		renderTargetDesc.LayerCount			= 1;
		renderTargetDesc.AddressMode		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		renderTargetDesc.Format				= format;
		renderTargetDesc.ImageType			= VK_IMAGE_TYPE_2D;

		renderTarget.Description			= renderTargetDesc;

		CreateImage		(renderTarget);
		CreateImageView	(renderTarget);

		renderTarget.ImageLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	}

	void GraphicsDevice::CreateRenderTarget(GPUImage& renderTarget, const RenderPassDesc& renderPassDesc, VkFormat format) {

		ImageDescription renderTargetDesc = {};
		renderTargetDesc.Width = renderPassDesc.Extent.width;
		renderTargetDesc.Height = renderPassDesc.Extent.height;
		renderTargetDesc.MipLevels = 1;
		//renderTargetDesc.MsaaSamples = m_MsaaSamples;
		//renderTargetDesc.MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
		renderTargetDesc.MsaaSamples = renderPassDesc.SampleCount;
		renderTargetDesc.Tiling = VK_IMAGE_TILING_OPTIMAL;
		//renderTargetDesc.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		renderTargetDesc.Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
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

	void GraphicsDevice::DestroyBuffer(GPUBuffer& buffer) {
		vkDestroyBuffer(m_LogicalDevice, buffer.Handle, nullptr);
		vkFreeMemory(m_LogicalDevice, buffer.Memory, nullptr);
	}

	template <class T>
	void GraphicsDevice::CopyDataFromStaging(GPUBuffer& dstBuffer, T* data, size_t dataSize, size_t offset) {
		BufferDescription stagingDesc = {};
		stagingDesc.Size = dataSize;
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
		desc.Capacity = bufferSize;
		desc.Size = 0;
		buffer.Description = desc;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = buffer.Description.Capacity;
		bufferInfo.usage = buffer.Description.Usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult result = vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &buffer.Handle);

		assert(result == VK_SUCCESS);

		AllocateMemory(buffer, buffer.Description.MemoryProperty);
	}

	Buffer GraphicsDevice::CreateBuffer(size_t size) {
		return m_BufferManager->SubAllocateBuffer(size);
	}

	GPUBuffer GraphicsDevice::CreateStorageBuffer(size_t size) {
		BufferDescription desc			= {};
		desc.Usage						= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		desc.Capacity					= size;
		desc.Size						= 0;
		desc.MemoryProperty				= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		GPUBuffer buffer				= {};
		buffer.Description = desc;

		VkBufferCreateInfo bufferInfo	= {};
		bufferInfo.sType				= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size					= buffer.Description.Capacity;
		bufferInfo.usage				= buffer.Description.Usage;
		bufferInfo.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;

		VkResult result					= vkCreateBuffer(m_LogicalDevice, &bufferInfo, nullptr, &buffer.Handle);

		assert(result == VK_SUCCESS);

		AllocateMemory(buffer, buffer.Description.MemoryProperty);

		return buffer;
	}

	// TODO: implement dynamic allocation/update/retrieval
	void GraphicsDevice::WriteBuffer(GPUBuffer& buffer, const void* data, size_t size, size_t offset) {
		if (data == nullptr)
			return;

		CopyDataFromStaging(buffer, data, std::min(buffer.Description.Capacity, size), offset);
	}

	void GraphicsDevice::WriteSubBuffer(Buffer& buffer, void* data, size_t dataSize) {
		m_BufferManager->WriteBuffer(buffer, data, dataSize);
	}

	void GraphicsDevice::UpdateBuffer(GPUBuffer& buffer, VkDeviceSize offset, void* data, size_t dataSize) {
		vkMapMemory(m_LogicalDevice, buffer.Memory, offset, dataSize, 0, &buffer.MemoryMapped);
		memcpy(buffer.MemoryMapped, data, dataSize);
		vkUnmapMemory(m_LogicalDevice, buffer.Memory);
	}

	void GraphicsDevice::UpdateBuffer(Buffer& buffer, void* data) {
		m_BufferManager->UpdateBuffer(buffer, data);
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

	void GraphicsDevice::CreateRenderPass(RenderPass& renderPass) {
	
		std::vector<VkAttachmentReference> colorAttachmentReferences;
		std::vector<VkAttachmentReference> depthAttachmentReferences;
		std::vector<VkAttachmentReference> resolveAttachmentReferences;

		if (renderPass.Description.Flags & eColorAttachment) {
			VkAttachmentDescription colorAttachment = {};
//			colorAttachment.format = m_SwapChain.ImageFormat;
			colorAttachment.format	= renderPass.Description.ColorImageFormat;
			colorAttachment.samples = renderPass.Description.SampleCount;

			if (renderPass.Description.Flags & eColorLoadOpLoad)
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			else if (renderPass.Description.Flags & eColorLoadOpClear)
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			else
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

			if (renderPass.Description.Flags & eColorStoreOpStore)
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			else
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			if (renderPass.Description.Flags & eInitialLayoutColorOptimal)
				colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			else
				colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (renderPass.Description.Flags & eFinalLayoutTransferSrc)
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			else if (renderPass.Description.Flags & eFinalLayoutTransferDst)
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			else if (renderPass.Description.Flags & eFinalLayoutPresent)
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			else
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			renderPass.Attachments.emplace_back(colorAttachment);

			VkAttachmentReference colorAttachRef = {};
			colorAttachRef.attachment = renderPass.Attachments.size() - 1;
			colorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			colorAttachmentReferences.emplace_back(colorAttachRef);

			renderPass.InitialLayout = colorAttachment.initialLayout;
			renderPass.FinalLayout = colorAttachment.finalLayout;
		}

		if (renderPass.Description.Flags & eDepthAttachment) {
			VkAttachmentDescription depthAttachment = {};
//			depthAttachment.format = GetDepthFormat();
			depthAttachment.format	= renderPass.Description.DepthImageFormat;
			depthAttachment.samples = renderPass.Description.SampleCount;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			renderPass.Attachments.emplace_back(depthAttachment);

			VkAttachmentReference depthAttachRef = {};
			depthAttachRef.attachment = renderPass.Attachments.size() - 1;
			depthAttachRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			depthAttachmentReferences.emplace_back(depthAttachRef);
		}

		if (renderPass.Description.Flags & eResolveAttachment) {
			VkAttachmentDescription resolveAttachment = {};
//			resolveAttachment.format = m_SwapChain.ImageFormat;
			resolveAttachment.format = renderPass.Description.ColorImageFormat;
			resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;


			if (renderPass.Description.Flags & eFinalLayoutTransferSrc)
				resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			else if (renderPass.Description.Flags & eFinalLayoutTransferDst)
				resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			else if (renderPass.Description.Flags & eFinalLayoutPresent)
				resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			else
				resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			renderPass.Attachments.emplace_back(resolveAttachment);

			VkAttachmentReference resolveAttachRef = {};
			resolveAttachRef.attachment = renderPass.Attachments.size() - 1;
			resolveAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			resolveAttachmentReferences.emplace_back(resolveAttachRef);


			renderPass.InitialLayout = resolveAttachment.initialLayout;
			renderPass.FinalLayout = resolveAttachment.finalLayout;
		}

		if (renderPass.Description.Flags & eFinalLayoutPresent) {
			VkSubpassDependency depthStencilDependency = {};
			depthStencilDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			depthStencilDependency.dstSubpass = 0;
			depthStencilDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			depthStencilDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			depthStencilDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			depthStencilDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			depthStencilDependency.dependencyFlags = 0;
			
			renderPass.Dependencies.emplace_back(depthStencilDependency);

			VkSubpassDependency colorDependency = {};
			colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			colorDependency.dstSubpass = 0;
			colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			colorDependency.srcAccessMask = 0;
			colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			colorDependency.dependencyFlags = 0;

			renderPass.Dependencies.emplace_back(colorDependency);
		}
		else {
			if (renderPass.Description.Flags & eDepthAttachment) {
				VkSubpassDependency dependency = {};
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				dependency.dstSubpass = 0;
				dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependency.srcAccessMask = VK_ACCESS_NONE_KHR;
				dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

				renderPass.Dependencies.emplace_back(dependency);

			}

			if (renderPass.Description.Flags & eColorAttachment || renderPass.Description.Flags & eFinalLayoutPresent) {
				VkSubpassDependency dependency = {};
				dependency.srcSubpass = 0;
				dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
				dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
				
				renderPass.Dependencies.emplace_back(dependency);
			}
		}

		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		
		subpassDesc.colorAttachmentCount = colorAttachmentReferences.size();
		subpassDesc.pColorAttachments = colorAttachmentReferences.data();
		subpassDesc.pDepthStencilAttachment = depthAttachmentReferences.data();
		subpassDesc.pResolveAttachments = resolveAttachmentReferences.data();

		renderPass.Subpasses.emplace_back(subpassDesc);

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(renderPass.Attachments.size());
		renderPassInfo.pAttachments = renderPass.Attachments.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(renderPass.Subpasses.size());
		renderPassInfo.pSubpasses = renderPass.Subpasses.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(renderPass.Dependencies.size());
		renderPassInfo.pDependencies = renderPass.Dependencies.data();

		VkResult result = vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &renderPass.Handle);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::DestroyRenderPass(VkRenderPass& renderPass) {
		vkDestroyRenderPass(m_LogicalDevice, renderPass, nullptr);
	}

	void GraphicsDevice::DestroyFramebuffer(std::vector<VkFramebuffer>& framebuffers) {
		for (int i = 0; i < framebuffers.size(); i++) {
			vkDestroyFramebuffer(m_LogicalDevice, framebuffers[i], nullptr);
		}
	}

	void GraphicsDevice::CreateDescriptorPool(const VkDescriptorPool& descriptorPool, const VkDescriptorPoolSize& poolSizes) {
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = 1;
		//poolCreateInfo.maxSets = 1;
		poolCreateInfo.maxSets = 256;
		poolCreateInfo.pPoolSizes = &poolSizes;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

		VkResult result = vkCreateDescriptorPool(m_LogicalDevice, &poolCreateInfo, nullptr, &m_DescriptorPool);

		assert(result == VK_SUCCESS);
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

	void GraphicsDevice::DestroyDescriptorPool(VkDescriptorPool& descriptorPool) {
		vkDestroyDescriptorPool(m_LogicalDevice, descriptorPool, nullptr);
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

	void GraphicsDevice::CreatePipelineLayout(const VkDescriptorSetLayout& descriptorSetLayout, VkPipelineLayout& pipelineLayout, const std::vector<VkPushConstantRange>& pushConstants) {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

		VkResult result = vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout);

		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::CreateDescriptorSetLayout(VkDescriptorSetLayout& layout, const std::vector<VkDescriptorSetLayoutBinding> bindings) {
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		
		VkResult result = vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &layout);
		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::CreateDescriptorSetLayout(VkDescriptorSetLayout& layout, const VkDescriptorSetLayoutCreateInfo& layoutInfo) {
		VkResult result = vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &layout);
		assert(result == VK_SUCCESS);
	}

	void GraphicsDevice::DestroyDescriptorSetLayout(VkDescriptorSetLayout& layout) {
			vkDestroyDescriptorSetLayout(m_LogicalDevice, layout, nullptr);
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

	void GraphicsDevice::CreateDescriptorSet(VkDescriptorPool& descriptorPool, VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSet& descriptorSet) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
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

	void GraphicsDevice::WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const GPUBuffer& buffer) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = buffer.Handle;
		bufferInfo.offset = 0;
		bufferInfo.range = buffer.Description.Capacity;

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

	void GraphicsDevice::WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const Buffer& buffer) {
		
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = *buffer.Handle;
		bufferInfo.offset = buffer.Offset;
		bufferInfo.range = buffer.Capacity;

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

	void GraphicsDevice::WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const GPUImage& image) {
		VkDescriptorImageInfo newImageInfo	= {};

		newImageInfo.imageLayout			= image.Description.AspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		newImageInfo.imageLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		newImageInfo.imageView				= image.ImageView;
		newImageInfo.sampler				= image.ImageSampler;

		VkWriteDescriptorSet descriptorWrite	= {};
		descriptorWrite.sType					= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet					= descriptorSet;
		descriptorWrite.dstBinding				= binding.binding;
		descriptorWrite.dstArrayElement			= 0;
		descriptorWrite.descriptorType			= binding.descriptorType;
		descriptorWrite.descriptorCount			= 1;
		descriptorWrite.pImageInfo				= &newImageInfo;
		
		vkUpdateDescriptorSets(m_LogicalDevice, 1, &descriptorWrite, 0, nullptr); 
	}

	void GraphicsDevice::WriteDescriptor(
		const VkDescriptorSetLayoutBinding binding, 
		const VkDescriptorSet& descriptorSet, 
		const VkImageLayout& imageLayout,
		const VkImageView& imageView,
		const VkSampler& imageSampler) {

		VkDescriptorImageInfo newImageInfo = {};
		//newImageInfo.imageLayout = image.ImageLayout;
		//newImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		newImageInfo.imageLayout = imageLayout;
		newImageInfo.imageView = imageView;
		newImageInfo.sampler = imageSampler;

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
#ifdef RUNTIME_SHADER_COMPILATION
		std::ifstream file(filename, std::ios::ate);
#else
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
#endif

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

#ifdef RUNTIME_SHADER_COMPILATION
	EShLanguage GraphicsDevice::FindLanguage(const Shader& shader) {
		switch (static_cast<VkShaderStageFlagBits>(shader.stage)) {
		case VK_SHADER_STAGE_VERTEX_BIT:
			return EShLangVertex;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return EShLangFragment;
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return EShLangGeometry;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return EShLangCompute;
		default:
			return EShLangVertex;
		}
	}

	static void InitResources(TBuiltInResource &Resources) {
		Resources.maxLights = 32;
		Resources.maxClipPlanes = 6;
		Resources.maxTextureUnits = 32;
		Resources.maxTextureCoords = 32;
		Resources.maxVertexAttribs = 64;
		Resources.maxVertexUniformComponents = 4096;
		Resources.maxVaryingFloats = 64;
		Resources.maxVertexTextureImageUnits = 32;
		Resources.maxCombinedTextureImageUnits = 80;
		Resources.maxTextureImageUnits = 32;
		Resources.maxFragmentUniformComponents = 4096;
		Resources.maxDrawBuffers = 32;
		Resources.maxVertexUniformVectors = 128;
		Resources.maxVaryingVectors = 8;
		Resources.maxFragmentUniformVectors = 16;
		Resources.maxVertexOutputVectors = 16;
		Resources.maxFragmentInputVectors = 15;
		Resources.minProgramTexelOffset = -8;
		Resources.maxProgramTexelOffset = 7;
		Resources.maxClipDistances = 8;
		Resources.maxComputeWorkGroupCountX = 65535;
		Resources.maxComputeWorkGroupCountY = 65535;
		Resources.maxComputeWorkGroupCountZ = 65535;
		Resources.maxComputeWorkGroupSizeX = 1024;
		Resources.maxComputeWorkGroupSizeY = 1024;
		Resources.maxComputeWorkGroupSizeZ = 64;
		Resources.maxComputeUniformComponents = 1024;
		Resources.maxComputeTextureImageUnits = 16;
		Resources.maxComputeImageUniforms = 8;
		Resources.maxComputeAtomicCounters = 8;
		Resources.maxComputeAtomicCounterBuffers = 1;
		Resources.maxVaryingComponents = 60;
		Resources.maxVertexOutputComponents = 64;
		Resources.maxGeometryInputComponents = 64;
		Resources.maxGeometryOutputComponents = 128;
		Resources.maxFragmentInputComponents = 128;
		Resources.maxImageUnits = 8;
		Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		Resources.maxCombinedShaderOutputResources = 8;
		Resources.maxImageSamples = 0;
		Resources.maxVertexImageUniforms = 0;
		Resources.maxTessControlImageUniforms = 0;
		Resources.maxTessEvaluationImageUniforms = 0;
		Resources.maxGeometryImageUniforms = 0;
		Resources.maxFragmentImageUniforms = 8;
		Resources.maxCombinedImageUniforms = 8;
		Resources.maxGeometryTextureImageUnits = 16;
		Resources.maxGeometryOutputVertices = 256;
		Resources.maxGeometryTotalOutputComponents = 1024;
		Resources.maxGeometryUniformComponents = 1024;
		Resources.maxGeometryVaryingComponents = 64;
		Resources.maxTessControlInputComponents = 128;
		Resources.maxTessControlOutputComponents = 128;
		Resources.maxTessControlTextureImageUnits = 16;
		Resources.maxTessControlUniformComponents = 1024;
		Resources.maxTessControlTotalOutputComponents = 4096;
		Resources.maxTessEvaluationInputComponents = 128;
		Resources.maxTessEvaluationOutputComponents = 128;
		Resources.maxTessEvaluationTextureImageUnits = 16;
		Resources.maxTessEvaluationUniformComponents = 1024;
		Resources.maxTessPatchComponents = 120;
		Resources.maxPatchVertices = 32;
		Resources.maxTessGenLevel = 64;
		Resources.maxViewports = 16;
		Resources.maxVertexAtomicCounters = 0;
		Resources.maxTessControlAtomicCounters = 0;
		Resources.maxTessEvaluationAtomicCounters = 0;
		Resources.maxGeometryAtomicCounters = 0;
		Resources.maxFragmentAtomicCounters = 8;
		Resources.maxCombinedAtomicCounters = 8;
		Resources.maxAtomicCounterBindings = 1;
		Resources.maxVertexAtomicCounterBuffers = 0;
		Resources.maxTessControlAtomicCounterBuffers = 0;
		Resources.maxTessEvaluationAtomicCounterBuffers = 0;
		Resources.maxGeometryAtomicCounterBuffers = 0;
		Resources.maxFragmentAtomicCounterBuffers = 1;
		Resources.maxCombinedAtomicCounterBuffers = 1;
		Resources.maxAtomicCounterBufferSize = 16384;
		Resources.maxTransformFeedbackBuffers = 4;
		Resources.maxTransformFeedbackInterleavedComponents = 64;
		Resources.maxCullDistances = 8;
		Resources.maxCombinedClipAndCullDistances = 8;
		Resources.maxSamples = 4;
		Resources.maxMeshOutputVerticesNV = 256;
		Resources.maxMeshOutputPrimitivesNV = 512;
		Resources.maxMeshWorkGroupSizeX_NV = 32;
		Resources.maxMeshWorkGroupSizeY_NV = 1;
		Resources.maxMeshWorkGroupSizeZ_NV = 1;
		Resources.maxTaskWorkGroupSizeX_NV = 32;
		Resources.maxTaskWorkGroupSizeY_NV = 1;
		Resources.maxTaskWorkGroupSizeZ_NV = 1;
		Resources.maxMeshViewCountNV = 4;
		Resources.limits.nonInductiveForLoops = 1;
		Resources.limits.whileLoops = 1;
		Resources.limits.doWhileLoops = 1;
		Resources.limits.generalUniformIndexing = 1;
		Resources.limits.generalAttributeMatrixVectorIndexing = 1;
		Resources.limits.generalVaryingIndexing = 1;
		Resources.limits.generalSamplerIndexing = 1;
		Resources.limits.generalVariableIndexing = 1;
		Resources.limits.generalConstantMatrixVectorIndexing = 1;
	}

	bool GraphicsDevice::CompileShader(Shader& shader) {
		glslang::InitializeProcess();

		EShLanguage stage = FindLanguage(shader);

		glslang::TShader compiledShader(stage);
		glslang::TProgram program;

		const char* shaderStrings[1];

		TBuiltInResource resources = {};
		InitResources(resources);

		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

		shaderStrings[0] = shader.sourceCode.data();
		compiledShader.setStrings(shaderStrings, 1);

		if (!compiledShader.parse(&resources, 100, false, messages)) {
			std::cout << compiledShader.getInfoLog() << '\n';
			std::cout << compiledShader.getInfoDebugLog() << '\n';
			return false;
		}

		program.addShader(&compiledShader);

		if (!program.link(messages)) {
			std::cout << compiledShader.getInfoLog() << '\n';
			std::cout << compiledShader.getInfoDebugLog() << '\n';
			return false;
		}

		glslang::GlslangToSpv(*program.getIntermediate(stage), shader.spirv);

		glslang::FinalizeProcess();

		return true;
	}
#endif

	void GraphicsDevice::LoadShader(VkShaderStageFlagBits shaderStage, Shader& shader, const std::string filename) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		shader.stage = shaderStage;
		shader.shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader.shaderStageInfo.stage = shaderStage;
		shader.shaderStageInfo.pName = "main";
		
		shader.sourceCode = ReadFile(filename);
#ifdef RUNTIME_SHADER_COMPILATION
		std::cout << "Compiling shader: " << filename << '\n';

		assert(CompileShader(shader));

		createInfo.codeSize = shader.spirv.size() * sizeof(unsigned int);
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.spirv.data());
#else
		std::cout << "Loading pre-compiled shader: " << filename << '\n';
		// already contains the SPIRV code
		createInfo.codeSize = shader.sourceCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.sourceCode.data());
#endif
	
		VkResult result = vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shader.shaderModule);
		assert(result == VK_SUCCESS);
		
		shader.shaderStageInfo.module = shader.shaderModule;
	}

	void GraphicsDevice::DestroyShader(Shader& shader) {
		if (shader.shaderModule == VK_NULL_HANDLE)
			return;

		vkDestroyShaderModule(m_LogicalDevice, shader.shaderModule, nullptr);
	}

	void GraphicsDevice::CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso, const IRenderTarget& renderTarget) {
	
		pso.inputAssembly.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pso.inputAssembly.topology					= desc.topology;
		pso.inputAssembly.primitiveRestartEnable	= VK_FALSE;

		VkExtent2D psoExtent	= renderTarget.GetExtent();
		VkViewport viewport		= renderTarget.GetRenderPass().Description.Viewport;
		VkRect2D scissor		= renderTarget.GetRenderPass().Description.Scissor;

		pso.viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pso.viewportState.viewportCount = 1;
		pso.viewportState.pViewports	= &viewport;
		pso.viewportState.scissorCount	= 1;
		pso.viewportState.pScissors		= &scissor;

		pso.rasterizer.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pso.rasterizer.depthClampEnable			= VK_FALSE;
		pso.rasterizer.rasterizerDiscardEnable	= VK_FALSE;
		pso.rasterizer.polygonMode				= desc.polygonMode;
		pso.rasterizer.lineWidth				= desc.lineWidth;
		pso.rasterizer.cullMode					= desc.cullMode;
		pso.rasterizer.frontFace				= desc.frontFace;
		pso.rasterizer.depthBiasEnable			= VK_FALSE;
		pso.rasterizer.depthBiasConstantFactor	= 0.0f;
		pso.rasterizer.depthBiasClamp			= 0.0f;
		pso.rasterizer.depthBiasSlopeFactor		= 0.0f;

		pso.multisampling.sType						= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pso.multisampling.sampleShadingEnable		= VK_FALSE;
		//pso.multisampling.rasterizationSamples	= m_MsaaSamples;
		pso.multisampling.rasterizationSamples		= renderTarget.GetRenderPass().Description.SampleCount;
		pso.multisampling.minSampleShading			= 1.0f;
		pso.multisampling.pSampleMask				= nullptr;
		pso.multisampling.alphaToCoverageEnable		= desc.colorBlendingEnable;
		pso.multisampling.alphaToOneEnable			= desc.colorBlendingEnable;

		desc.colorBlendingDesc.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		
		pso.colorBlending.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pso.colorBlending.logicOpEnable		= VK_FALSE;
		pso.colorBlending.logicOp			= VK_LOGIC_OP_COPY;
		pso.colorBlending.attachmentCount	= 1;
		pso.colorBlending.pAttachments		= &desc.colorBlendingDesc;
		pso.colorBlending.blendConstants[0] = 0.0f;
		pso.colorBlending.blendConstants[1] = 0.0f;
		pso.colorBlending.blendConstants[2] = 0.0f;
		pso.colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState	= {};
		dynamicState.sType								= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount					= static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates						= dynamicStates.data();

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		VkVertexInputBindingDescription bindingDescription						= Assets::Vertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions	= Assets::Vertex::GetAttributeDescriptions();

		if (desc.vertexShader != nullptr) {

			pso.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			if (desc.noVertex) {
				pso.vertexInputInfo.vertexBindingDescriptionCount = 0;
			}
			else {
				pso.vertexInputInfo.vertexBindingDescriptionCount	= 1;
				pso.vertexInputInfo.pVertexBindingDescriptions		= &bindingDescription;
				pso.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
				pso.vertexInputInfo.pVertexAttributeDescriptions	= attributeDescriptions.data();
			}
			pso.pipelineInfo.pVertexInputState = &pso.vertexInputInfo;
			
			shaderStages.push_back(desc.vertexShader->shaderStageInfo);
		}

		if (desc.fragmentShader != nullptr)
			shaderStages.push_back(desc.fragmentShader->shaderStageInfo);
		if (desc.computeShader != nullptr)
			shaderStages.push_back(desc.computeShader->shaderStageInfo);
		if (desc.geometryShader != nullptr)
			shaderStages.push_back(desc.geometryShader->shaderStageInfo);

		for (auto stage : shaderStages) {
			stage.pNext = nullptr;
			stage.pSpecializationInfo = nullptr;
		}

		pso.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pso.depthStencil.depthTestEnable = desc.depthTestEnable ? VK_TRUE : VK_FALSE;
		pso.depthStencil.depthWriteEnable = desc.depthWriteEnable ? VK_TRUE : VK_FALSE;
		pso.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		pso.depthStencil.depthBoundsTestEnable = VK_FALSE;
		pso.depthStencil.minDepthBounds = 0.0f;
		pso.depthStencil.maxDepthBounds = 1.0f;
		pso.depthStencil.stencilTestEnable = desc.stencilTestEnable ? VK_TRUE : VK_FALSE;

		pso.depthStencil.back = desc.stencilState;
		pso.depthStencil.front = pso.depthStencil.back;

		for (auto inputLayout : desc.psoInputLayout) {
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;

			CreateDescriptorSetLayout(layout, inputLayout.bindings);

			pso.layoutBindings.insert(pso.layoutBindings.end(), inputLayout.bindings.begin(), inputLayout.bindings.end());
			pso.pushConstants.insert(pso.pushConstants.end(), inputLayout.pushConstants.begin(), inputLayout.pushConstants.end());

			pso.descriptorSetLayout.push_back(layout);

			/*
			size_t binding_hash = 0;

			for (const auto& binding : inputLayout.bindings) {
				Helper::hash_combine(binding_hash, binding.binding);
				Helper::hash_combine(binding_hash, binding.descriptorCount);
				Helper::hash_combine(binding_hash, binding.descriptorType);
				Helper::hash_combine(binding_hash, binding.stageFlags);
			}	

			if (m_Frames[0].descriptorSets.find(binding_hash) == m_Frames[0].descriptorSets.end()) {
				for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
					CreateDescriptorSet(layout, m_Frames[i].descriptorSets[binding_hash]);
				}
			}
			*/
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo	= {};
		pipelineLayoutInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount				= static_cast<uint32_t>(pso.descriptorSetLayout.size());
		pipelineLayoutInfo.pSetLayouts					= pso.descriptorSetLayout.data();
		pipelineLayoutInfo.pushConstantRangeCount		= static_cast<uint32_t>(pso.pushConstants.size());
		pipelineLayoutInfo.pPushConstantRanges			= pso.pushConstants.data();

		VkResult result = vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &pso.pipelineLayout);
		assert(result == VK_SUCCESS);


		pso.pipelineInfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pso.pipelineInfo.stageCount				= static_cast<uint32_t>(shaderStages.size());
		pso.pipelineInfo.pStages				= shaderStages.data();
		pso.pipelineInfo.pInputAssemblyState	= &pso.inputAssembly;
		pso.pipelineInfo.pViewportState			= &pso.viewportState;
		pso.pipelineInfo.pRasterizationState	= &pso.rasterizer;
		pso.pipelineInfo.pMultisampleState		= &pso.multisampling;
		pso.pipelineInfo.pDepthStencilState		= &pso.depthStencil;
		pso.pipelineInfo.pColorBlendState		= &pso.colorBlending;
		pso.pipelineInfo.pDynamicState			= &dynamicState;
		pso.pipelineInfo.layout					= pso.pipelineLayout;
		pso.pipelineInfo.renderPass				= renderTarget.GetRenderPass().Handle;
		pso.pipelineInfo.subpass				= 0;
		pso.pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
		//pipelineInfo.basePipelineIndex		= -1;

		pso.renderPass							= &renderTarget.GetRenderPass();

		result = vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pso.pipelineInfo, nullptr, &pso.pipeline);
		assert(result == VK_SUCCESS);

		pso.description = desc;
	}

	void GraphicsDevice::DestroyPipelineLayout(VkPipelineLayout& pipelineLayout) {
		if (pipelineLayout == VK_NULL_HANDLE)
			return;

		vkDestroyPipelineLayout(m_LogicalDevice, pipelineLayout, nullptr);
	}

	void GraphicsDevice::DestroyPipeline(PipelineState& pso) {
		if (pso.pipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(m_LogicalDevice, pso.pipelineLayout, nullptr);

		pso.pushConstants.clear();
		pso.layoutBindings.clear();
		pso.imageViewTypes.clear();

		for (auto descriptorSetLayout : pso.descriptorSetLayout) {
			vkDestroyDescriptorSetLayout(m_LogicalDevice, descriptorSetLayout, nullptr);
		}

		pso.descriptorSetLayout.clear();

		if (pso.pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(m_LogicalDevice, pso.pipeline, nullptr);
	}

	Frame& GraphicsDevice::GetCurrentFrame() {
		return m_Frames[m_CurrentFrame % Graphics::FRAMES_IN_FLIGHT];
	}

	Frame& GraphicsDevice::GetLastFrame() {
		return m_Frames[(m_CurrentFrame - 1) % Graphics::FRAMES_IN_FLIGHT];
	}

	Frame& GraphicsDevice::GetFrame(int i) {
		return m_Frames[i];
	}

	void GraphicsDevice::DestroyRenderPass(RenderPass& renderPass) {
		//DestroyImage(renderPass.depthBuffer);
		//DestroyImage(renderPass.renderTarget);

		renderPass.Attachments.clear();
		renderPass.Dependencies.clear();
		renderPass.Subpasses.clear();

		vkDestroyRenderPass(m_LogicalDevice, renderPass.Handle, nullptr);
	}

	void GraphicsDevice::ResizeRenderPass(const uint32_t width, const uint32_t height, RenderPass& renderPass) {
		// TODO: need to rework framebuffer resize

		/*
		float widthRatio = static_cast<float>(width) / static_cast<float>(renderPass.Description.extent.width);
		float heightRatio = static_cast<float>(height) / static_cast<float>(renderPass.Description.extent.height);

		uint32_t newWidth = static_cast<uint32_t>(renderPass.Description.extent.width * widthRatio);
		uint32_t newHeight = static_cast<uint32_t>(renderPass.Description.extent.height * heightRatio);

		renderPass.Description.extent.width = newWidth;
		renderPass.Description.extent.height = newHeight;

		renderPass.Description.viewport.width = static_cast<float>(newWidth);
		renderPass.Description.viewport.height = static_cast<float>(newHeight);
		renderPass.Description.scissor.extent = { newWidth, newHeight };

		DestroyFramebuffer(renderPass.framebuffers);
		
		for (int i = 0; i < renderPass.framebuffers.size(); i++) {
			std::vector<VkImageView> framebufferAttachments = {};

			if (renderPass.Description.Flags & RenderPass::tColorAttachment)
				framebufferAttachments.emplace_back(Graphics::g_SceneColor);
			if (renderPass.Description.Flags & RenderPass::tDepthAttachment)
				framebufferAttachments.emplace_back(Graphics::g_SceneDepth);
			if (renderPass.Description.Flags & RenderPass::tColorResolveAttachment)
				framebufferAttachments.emplace_back(m_SwapChain.ImageViews[i]);

			CreateFramebuffer(
				renderPass.Handle, 
				framebufferAttachments, 
				renderPass.Description.extent, 
				renderPass.framebuffers[i]);
		}
		*/
	}
}
