#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <random>

#include <chrono>
#include <memory>

#include "Common.h"
#include "UI.h"
#include "UserSettings.h"
#include "WindowSettings.h"

#ifndef NDEBUG
const bool c_EnableValidationLayers = true;
#else
const bool c_EnableValidationLayers = false;
#endif

const std::vector<const char*> c_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

namespace Engine {
	class Application {
	public:
		Application(const WindowSettings &windowSettings = WindowSettings(), 
			const UserSettings &userSettings = UserSettings());
		~Application();

		void Run();
		void Close();
	private:
		void Init();
		void InitGlfw();
		void InitVulkan();
		void Shutdown();

		void createVulkanInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createSwapChain();
		void createImageViews();
		void createRenderPass();
		void createDescriptorSetLayout();
		//void createGraphicsPipeline();
		void createGraphicsPipeline(const char* vertexShaderPath, const char* fragShaderPath,
			VkVertexInputBindingDescription desiredBindingDescription, std::array<VkVertexInputAttributeDescription, 2> desiredAttributeDescriptions,
			VkPrimitiveTopology topology, VkPipelineLayout& r_PipelineLayout, VkPipeline& r_GraphicsPipeline,
			uint32_t layoutCount, VkDescriptorSetLayout &layout);
		void createFramebuffers();
		void createCommandPool();
		void createVertexBuffer();
		void createIndexBuffer();
		void createUniformBuffers();
		void createDescriptorPool();
		void createComputeDescriptorPool();
		void createDescriptorSets();
		void createCommandBuffers();
		void createSyncObjects();
		void recreateSwapChain();
		void cleanupSwapChain();

		void createShaderStorageBuffers();
		void createComputeDescriptorSets();
		void createComputeDescriptorSetLayout();
		void createComputePipeline();
		
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void updateUniformBuffer(uint32_t currentImage);
		void updateVertexBuffer(uint32_t currentImage);
		void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
		void createComputeCommandBuffers();

		void drawFrame();
		void handleDraw(uint32_t imageIndex);
		void drawRayTraced(VkCommandBuffer &r_CommandBuffer, uint32_t imageIndex);
		void drawRasterized(VkCommandBuffer &r_CommandBuffer, uint32_t imageIndex);

	private:
		GLFWwindow* m_Window;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkSwapchainKHR m_SwapChain;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		VkFormat m_SwapChainImageFormat;
		VkExtent2D m_SwapChainExtent;
		VkRenderPass m_RenderPass;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkPipelineLayout m_RayTracerPipelineLayout;
		VkPipelineLayout m_RasterizerPipelineLayout;
		VkPipeline m_RayTracerGraphicsPipeline;
		VkPipeline m_RasterizerGraphicsPipeline;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;
		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		WindowSettings m_WindowSettings;
		UserSettings m_UserSettings;
		uint32_t m_CurrentFrame = 0;
		std::vector<VkBuffer> m_VertexBuffers;
		std::vector<VkDeviceMemory> m_VertexBuffersMemory;
		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;
		std::vector<VkBuffer> m_UniformBuffers;
		std::vector<VkDeviceMemory> m_UniformBuffersMemory;
		std::vector<void*> m_UniformBuffersMapped;
		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		std::vector<VkBuffer> m_ShaderStorageBuffers;
		std::vector<VkDeviceMemory> m_ShaderStorageBuffersMemory;

		VkDescriptorPool m_ComputeDescriptorPool;
		VkDescriptorSetLayout m_ComputeDescriptorSetLayout;
		std::vector<VkDescriptorSet> m_ComputeDescriptorSets;
		VkPipelineLayout m_ComputePipelineLayout;
		VkPipeline m_ComputePipeline;
		std::vector<VkFence> m_ComputeInFlightFences;
		std::vector<VkSemaphore> m_ComputeFinishedSemaphores;
		std::vector<VkCommandBuffer> m_ComputeCommandBuffers;
	};
}
