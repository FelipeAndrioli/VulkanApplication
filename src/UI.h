#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "Vulkan.h"

namespace Engine {

	class UI {
	public:
		UI(GLFWwindow& window);
		~UI() {};

		void BeginFrame();
		void EndFrame(const VkCommandBuffer& commandBuffer);
		void Shutdown(LogicalDevice& logicalDevice);
	private:
		void createUIDescriptorPool(VkDevice& r_LogicalDevice);
		void createUICommandPool(VkDevice& r_LogicalDevice, const uint32_t queueFamilyIndex);
		
	private:
		VkDescriptorPool m_UIDescriptorPool;
		VkCommandPool m_UICommandPool;
	};
};
