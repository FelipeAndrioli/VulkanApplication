#include "LogicalDevice.h"

namespace Engine {
	LogicalDevice::LogicalDevice(Instance* instance, PhysicalDevice* physicalDevice) : p_Instance(instance), p_PhysicalDevice(physicalDevice) {
		QueueFamilyIndices indices = p_PhysicalDevice->GetQueueFamilyIndices();

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

		if (p_Instance->GetIsEnabledValidationLayers()) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(p_Instance->GetValidationLayers().size());
			createInfo.ppEnabledLayerNames = p_Instance->GetValidationLayers().data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(p_PhysicalDevice->GetHandle(), &createInfo, nullptr, &m_VulkanDevice) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device");
		}

		vkGetDeviceQueue(m_VulkanDevice, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_VulkanDevice, indices.presentFamily.value(), 0, &m_PresentQueue);
		vkGetDeviceQueue(m_VulkanDevice, indices.graphicsAndComputeFamily.value(), 0, &m_ComputeQueue);

	}

	LogicalDevice::~LogicalDevice() {
		vkDestroyDevice(m_VulkanDevice, nullptr);
	}

	void LogicalDevice::WaitIdle() {
		vkDeviceWaitIdle(m_VulkanDevice);
	}
}