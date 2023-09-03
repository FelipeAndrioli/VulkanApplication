#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Vulkan.h"
#include "Common.h"
#include "UserSettings.h"
#include "WindowSettings.h"

namespace Engine {
	class UI {
	public:
		UI();
		~UI();

		void Init(GLFWwindow& r_Window, const VkInstance& r_VulkanInstance, const VkPhysicalDevice& r_PhysicalDevice,
			VkDevice& r_LogicalDevice, const QueueFamilyIndices& r_QueueFamilyIndices, VkQueue& r_GraphicsQueue,
			const VkExtent2D& r_SwapChainExtent, const std::vector<VkImageView>& r_SwapChainImageViews,
			const VkFormat& r_SwapChainImageFormat, const int minImageCount);
		void Destroy(VkDevice &r_LogicalDevice);
		void RecordCommands(const VkExtent2D &r_SwapChainExtent, const uint32_t currentFrame, const uint32_t imageIndex);
		void Resize(VkDevice& r_LogicalDevice, const VkExtent2D& r_SwapChainExtent, const std::vector<VkImageView>& r_SwapChainImageViews,
			int minImageCount);
		VkCommandBuffer& GetCommandBuffer(uint32_t currentFrame);
		void Draw(UserSettings& r_UserSettings, WindowSettings& r_WindowSettings);
	private:
		void createUIDescriptorPool(VkDevice& r_LogicalDevice);
		void createUIRenderPass(VkDevice& r_LogicalDevice, const VkFormat& r_SwapChainImageFormat);
		void createUICommandPool(VkDevice& r_LogicalDevice, const uint32_t queueFamilyIndex);
		void createUICommandBuffers(VkDevice& r_LogicalDevice);
		void createUIFrameBuffers(VkDevice& r_LogicalDevice, const VkExtent2D& r_SwapChainExtent,
			const std::vector<VkImageView>& r_SwapChainImageViews);
		
	private:
		VkDescriptorPool m_UIDescriptorPool;
		VkRenderPass m_UIRenderPass;
		VkCommandPool m_UICommandPool;
		std::vector<VkCommandBuffer> m_UICommandBuffers;
		std::vector<VkFramebuffer> m_UIFramebuffers;
	};
};
