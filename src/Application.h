#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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
#include "DescriptorSets.h"
#include "ComputePipeline.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include "BufferHelper.h"
#include "Semaphore.h"
#include "Fence.h"
#include "Instance.h"
#include "DebugUtilsMessenger.h"
#include "ResourceSet.h"
#include "DepthBuffer.h"

#include "../Assets/Scene.h"

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

		void Init();
		void SetActiveScene(Assets::Scene* scene);
		void Run();
	private:
		void Update(float t);
		void InitVulkan();
		void Shutdown();

		void createSyncObjects();
		void recreateSwapChain();

		void updateComputeUniformBuffer(uint32_t currentImage);
		void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

		void drawFrame();
		void handleDraw(uint32_t imageIndex);
		void drawRayTraced(VkCommandBuffer &r_CommandBuffer, uint32_t imageIndex);
		void drawRasterized(const VkCommandBuffer& commandBuffer, uint32_t imageIndex);
		
		void processKey(int key, int scancode, int action, int mods);
		void processResize(int width, int height);

		void CreateFramebuffers(const VkRenderPass& renderPass);
		void ClearFramebuffers();
		
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

		std::unique_ptr<class CommandPool> m_CommandPool;
		std::unique_ptr<class DebugUtilsMessenger> m_DebugMessenger;
		std::unique_ptr<class Semaphore> m_ImageAvailableSemaphores;
		std::unique_ptr<class Semaphore> m_RenderFinishedSemaphores;
		std::unique_ptr<class Semaphore> m_ComputeFinishedSemaphores;
		std::unique_ptr<class Fence> m_InFlightFences;
		std::unique_ptr<class Fence> m_ComputeInFlightFences;

		std::unique_ptr<class DepthBuffer> m_DepthBuffer;

		std::unique_ptr<class RenderPass> m_DefaultRenderPass;
		std::vector<VkFramebuffer> m_Framebuffers;

		std::unique_ptr<class UI> m_UI;
		
		WindowSettings m_WindowSettings;
		UserSettings m_UserSettings;
		uint32_t m_CurrentFrame = 0;

		std::unique_ptr<class CommandBuffer> m_CommandBuffers;
		std::unique_ptr<class Buffer> m_ComputeUniformBuffers;
		std::unique_ptr<class CommandBuffer> m_ComputeCommandBuffers;

		std::unique_ptr<class Buffer> m_VertexBuffers;
		std::unique_ptr<class Buffer> m_IndexBuffer;

		std::unique_ptr<class Buffer> m_ShaderStorageBuffers;

		Assets::Scene* p_ActiveScene = nullptr;
	};
}
