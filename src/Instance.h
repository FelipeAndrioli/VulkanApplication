#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <set>

#include "VulkanHeader.h"
#include "Common.h"

namespace Engine {
	class Instance {
	public:
		Instance(const std::vector<const char*> &validationLayers, const bool &enableValidationLayers);
		~Instance();

		inline VkInstance& GetHandle() { return m_VulkanInstance; };
		inline const std::vector<const char*>& GetValidationLayers() { return m_ValidationLayers; };
		inline bool GetIsEnabledValidationLayers() { return m_EnabledValidationLayers; };
	private:

		const std::vector<const char*> m_ValidationLayers;
		const bool m_EnabledValidationLayers;
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
	private:
		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();
		void checkRequiredExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions,
			std::vector<VkExtensionProperties> vulkanSupportedExtensions);
	};
}
