#pragma once

#include <iostream>
#include <stdexcept>

#include "Vulkan.h"
#include "Instance.h"

namespace Engine {
	class DebugUtilsMessenger {
	public:
		DebugUtilsMessenger(const VkInstance& instance);
		~DebugUtilsMessenger();
	private:
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	private:
		const VkInstance& r_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
	};
}
