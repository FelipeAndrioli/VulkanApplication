#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <set>

#include "Vulkan.h"
#include "Common.h"

namespace Engine {
	class Instance {
	public:
		Instance(const std::vector<const char*> &validationLayers, const bool &enableValidationLayers);
		~Instance();

		VkInstance& GetHandle();
		const std::vector<const char*>& GetValidationLayers();
	private:

		const std::vector<const char*> m_ValidationLayers;
		const bool m_EnableValidationLayers;
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
	private:
		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();
		void checkRequiredExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions,
			std::vector<VkExtensionProperties> vulkanSupportedExtensions);
	};
}
