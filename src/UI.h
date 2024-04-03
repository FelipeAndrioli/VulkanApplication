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
	class RenderPass;

	struct Settings;

	class UI {
	/*
		TODO: rework this entire thing in the future	
	*/

	public:
		UI(GLFWwindow* window, Instance* instance, PhysicalDevice* physicalDevice, LogicalDevice* logicalDevice,
			SwapChain* swapChain, RenderPass* renderPass, CommandBuffer* commandBuffer, const int minImageCount);
		~UI();

		void Init();
		void Resize(SwapChain* swapChain);
		void BeginFrame();
		void EndFrame(const VkCommandBuffer& commandBuffer);
	private:
		void createUIDescriptorPool(VkDevice& r_LogicalDevice);
		void createUICommandPool(VkDevice& r_LogicalDevice, const uint32_t queueFamilyIndex);
		
	private:
		VkDescriptorPool m_UIDescriptorPool;
		//VkRenderPass m_UIRenderPass;
		VkCommandPool m_UICommandPool;
		//std::vector<VkCommandBuffer> m_UICommandBuffers;
		//std::vector<VkFramebuffer> m_UIFramebuffers;

		GLFWwindow* p_Window;
		Instance* p_Instance;
		PhysicalDevice* p_PhysicalDevice;
		LogicalDevice* p_LogicalDevice;
		SwapChain* p_SwapChain;
		RenderPass* p_RenderPass;
		CommandBuffer* p_CommandBuffer;

		int m_MinImageCount;
	};
};
