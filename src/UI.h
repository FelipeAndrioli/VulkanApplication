#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Vulkan.h"

namespace Assets {
	class Scene;
}

namespace Engine {
	class CommandBuffer;
	class Instance;
	class PhysicalDevice;
	class LogicalDevice;
	class SwapChain;

	struct Settings;

	class UI {
	public:
		UI(GLFWwindow* window, Instance* instance, PhysicalDevice* physicalDevice, LogicalDevice* logicalDevice,
			SwapChain* swapChain, const int minImageCount);
		~UI();

		void Init();
		void RecordCommands(const uint32_t currentFrame, const uint32_t imageIndex);
		void Resize(SwapChain* swapChain);
		VkCommandBuffer& GetCommandBuffer(uint32_t currentFrame);
		void Draw(Settings& settings, Assets::Scene* scene);
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

		GLFWwindow* p_Window;
		Instance* p_Instance;
		PhysicalDevice* p_PhysicalDevice;
		LogicalDevice* p_LogicalDevice;
		SwapChain* p_SwapChain;

		int m_MinImageCount;
	};
};
