#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace Engine {

	class UI {
	public:
		UI(GLFWwindow& window);
		~UI();

		void BeginFrame();
		void EndFrame(const VkCommandBuffer& commandBuffer);
	private:
		void createUIDescriptorPool(VkDevice& r_LogicalDevice);
		void createUICommandPool(VkDevice& r_LogicalDevice, const uint32_t queueFamilyIndex);

		static void check_vk_result(VkResult err) {
			if (err == 0) return;
			fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
			if (err < 0) throw std::runtime_error("Something went wrong");
		}
	private:
		VkDescriptorPool m_UIDescriptorPool;
		VkCommandPool m_UICommandPool;
	};
};
