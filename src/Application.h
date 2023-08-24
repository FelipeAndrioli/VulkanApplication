#pragma once

#define GLM_FORCE_RADIANS

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
#include <functional>

#include <chrono>
#include <memory>

#include "Vulkan.h"
#include "Common.h"
#include "UI.h"
#include "UserSettings.h"
#include "WindowSettings.h"
#include "Window.h"
#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "Semaphore.h"
#include "Fence.h"
#include "Instance.h"
#include "DebugUtilsMessenger.h"

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
	private:
		void Init();
		void InitVulkan();
		void Shutdown();

		void createDescriptorSetLayout();
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
	
		void processKey(int key, int scancode, int action, int mods);
		void processResize(int width, int height);

	private:
		std::unique_ptr<class Window> m_Window;
		std::unique_ptr<class Instance> m_Instance;
		std::unique_ptr<class Surface> m_Surface;
		std::unique_ptr<class PhysicalDevice> m_PhysicalDevice;
		std::unique_ptr<class LogicalDevice> m_LogicalDevice;
		std::unique_ptr<class SwapChain> m_SwapChain;
		std::unique_ptr<class GraphicsPipeline> m_GraphicsPipeline;
		std::unique_ptr<class GraphicsPipeline> m_TempRayTracerPipeline;
		std::unique_ptr<class ComputePipeline> m_ComputePipeline;
		std::unique_ptr<class DebugUtilsMessenger> m_DebugMessenger;
		std::unique_ptr<class Semaphore> m_ImageAvailableSemaphores;
		std::unique_ptr<class Semaphore> m_RenderFinishedSemaphores;
		std::unique_ptr<class Semaphore> m_ComputeFinishedSemaphores;
		std::unique_ptr<class Fence> m_InFlightFences;
		std::unique_ptr<class Fence> m_ComputeInFlightFences;

		/*
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkPipelineLayout m_RayTracerPipelineLayout;
		VkPipelineLayout m_RasterizerPipelineLayout;
		VkPipeline m_RayTracerGraphicsPipeline;
		VkPipeline m_RasterizerGraphicsPipeline;
		*/

		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;
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
		std::vector<VkDescriptorSet> m_ComputeDescriptorSets;
		std::vector<VkCommandBuffer> m_ComputeCommandBuffers;
	};
}
