#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "VulkanHeader.h"

namespace Engine {
	class CommandBuffer;
	class Instance;
	class PhysicalDevice;
	class LogicalDevice;
	class RenderPass;

	class UI {
	public:
		UI(
			GLFWwindow& window, 
			Instance& instance, 
			PhysicalDevice& physicalDevice, 
			LogicalDevice& logicalDevice,
			RenderPass& renderPass, 
			const int minImageCount
		);
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
