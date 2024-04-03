#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

#ifndef NDEBUG
const bool c_EnableValidationLayers = true;
#else
const bool c_EnableValidationLayers = false;
#endif

const std::vector<const char*> c_ValidationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

/*
namespace Engine {
	class Image;
	class LogicalDevice;
	class PhysicalDevice;
	class SwapChain;

	class RenderTarget {
	public:
		RenderTarget(
			LogicalDevice& logicalDevice, 
			PhysicalDevice& physicalDevice, 
			SwapChain& swapChain, 
			const VkSampleCountFlagBits msaaSamples
		);
		~RenderTarget();

		std::unique_ptr<class Image> m_RenderTarget;
	};
}
*/

namespace Engine {

	class Window;
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class SwapChain;
	class Instance;
	class Fence;
	class RenderPass;
	class DepthBuffer;
	class Semaphore;
	struct Settings;

	class VulkanEngine {
	public:
		VulkanEngine(Window& window, Settings& settings);
		~VulkanEngine();

		void Resize();

		LogicalDevice& GetLogicalDevice() { return *m_LogicalDevice.get(); }
		PhysicalDevice& GetPhysicalDevice() { return *m_PhysicalDevice.get(); }
		CommandPool& GetCommandPool() { return *m_CommandPool.get(); }
		SwapChain& GetSwapChain() { return *m_SwapChain.get(); }
		Instance& GetInstance() { return *m_Instance.get(); }
		Fence& GetInFlightFences() { return *m_InFlightFences.get(); }
		RenderPass& GetDefaultRenderPass() { return *m_DefaultRenderPass.get(); }
		VkFramebuffer& GetFramebuffer(uint32_t index) { return m_Framebuffers[index]; }
		DepthBuffer& GetDepthbuffer() { return *m_DepthBuffer.get(); }
		Semaphore& GetImageAvailableSemaphores() { return *m_ImageAvailableSemaphores.get(); }

		VkResult PrepareNextImage(uint32_t& currentFrame, uint32_t& imageIndex);
		VkCommandBuffer* BeginFrame(uint32_t& currentFrame, uint32_t& imageIndex);
		void EndFrame(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, uint32_t imageIndex);
		void PresentFrame(uint32_t& currentFrame, uint32_t& imageIndex);
	
		void BeginUIFrame();
		void EndUIFrame(const VkCommandBuffer& commandBuffer);
	private:
		std::unique_ptr<class DebugUtilsMessenger> m_DebugMessenger;
		std::unique_ptr<class Instance> m_Instance;
		std::unique_ptr<class Surface> m_Surface;
		std::unique_ptr<class PhysicalDevice> m_PhysicalDevice;
		std::unique_ptr<class LogicalDevice> m_LogicalDevice;
		std::unique_ptr<class SwapChain> m_SwapChain;

		//std::unique_ptr<class RenderTarget> m_RenderTarget;
		std::unique_ptr<class Image> m_RenderTarget;

		std::unique_ptr<class DepthBuffer> m_DepthBuffer;
		std::unique_ptr<class RenderPass> m_DefaultRenderPass;
		std::vector<VkFramebuffer> m_Framebuffers;
		std::unique_ptr<class CommandPool> m_CommandPool;
		std::unique_ptr<class CommandBuffer> m_CommandBuffers;
		std::unique_ptr<class Semaphore> m_ImageAvailableSemaphores;
		std::unique_ptr<class Semaphore> m_RenderFinishedSemaphores;
		std::unique_ptr<class Semaphore> m_ComputeFinishedSemaphores;
		std::unique_ptr<class Fence> m_InFlightFences;
		std::unique_ptr<class Fence> m_ComputeInFlightFences;
		std::unique_ptr<class UI> m_UI;

	private:
		void CreateFramebuffers(const VkRenderPass& renderPass);
		void ClearFramebuffers();
		void CreateSyncObjects();
	};
}

