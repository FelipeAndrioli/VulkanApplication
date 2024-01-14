#pragma once

#include <iostream>
#include <vector>
#include <map>

#include "Vulkan.h"
#include "Settings.h"

#ifndef NDEBUG
const bool c_EnableValidationLayers = true;
#else
const bool c_EnableValidationLayers = false;
#endif

const std::vector<const char*> c_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

namespace Assets {
	class Scene;
}

namespace Engine {
	namespace InputSystem {
		class Input;
	}

	class Window;
	class Instance;
	class Surface;
	class PhysicalDevice;
	class LogicalDevice;
	class SwapChain;
	class GraphicsPipeline;
	class CommandPool;
	class DebugUtilsMessenger;
	class Semaphore;
	class Semaphore;
	class Semaphore;
	class Fence;
	class Fence;
	class DepthBuffer;
	class RenderPass;
	class UI;
	class CommandBuffer;
	class Buffer;
	class CommandBuffer;
	class Buffer;
	class Material;

	struct Settings;

	class Application {
	public:
		Application(const Settings &settings = Settings());
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
		//void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

		void Draw();
		VkCommandBuffer* BeginFrame();
		void DrawFrame(const VkCommandBuffer& commandBuffer);
		void EndFrame(const VkCommandBuffer& commandBuffer);
		void PresentFrame();

		void drawRayTraced(VkCommandBuffer &r_CommandBuffer, uint32_t imageIndex);
		
		void ProcessResize(int width, int height);

		void CreateFramebuffers(const VkRenderPass& renderPass);
		void ClearFramebuffers();
		
	private:
		std::unique_ptr<class Window> m_Window;
		std::unique_ptr<class Instance> m_Instance;
		std::unique_ptr<class Surface> m_Surface;
		std::unique_ptr<class PhysicalDevice> m_PhysicalDevice;
		std::unique_ptr<class LogicalDevice> m_LogicalDevice;
		std::unique_ptr<class SwapChain> m_SwapChain;

		std::unique_ptr<class GraphicsPipeline> m_TempRayTracerPipeline;
		//std::unique_ptr<class ComputePipeline> m_ComputePipeline;

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
	
		std::unique_ptr<class InputSystem::Input> m_Input;

		Settings m_Settings;

		uint32_t m_CurrentFrame = 0;
		uint32_t m_ImageIndex = 0;

		std::unique_ptr<class CommandBuffer> m_CommandBuffers;
		std::unique_ptr<class Buffer> m_ComputeUniformBuffers;
		std::unique_ptr<class CommandBuffer> m_ComputeCommandBuffers;

		std::unique_ptr<class Buffer> m_VertexBuffers;
		std::unique_ptr<class Buffer> m_IndexBuffer;

		std::unique_ptr<class Buffer> m_ShaderStorageBuffers;

		Assets::Scene* p_ActiveScene = nullptr;

		bool m_FramebufferResized = false;

		std::map<std::string, std::unique_ptr<class GraphicsPipeline>> m_GraphicsPipelines;
		std::unique_ptr<std::map<std::string, std::unique_ptr<class Material>>> m_Materials;
	};
}
