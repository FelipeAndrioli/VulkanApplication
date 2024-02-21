#pragma once

#include <iostream>
#include <vector>
#include <map>

#include <glm/glm.hpp>

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
	class Material;

	struct Texture;
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

	struct Settings;

	struct ObjectGPUData {
		glm::mat4 model = glm::mat4(1.0f);
	};

	struct SceneGPUData {
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);
	};

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
		void Draw();
		VkCommandBuffer* BeginFrame();
		void DrawFrame(const VkCommandBuffer& commandBuffer);
		void EndFrame(const VkCommandBuffer& commandBuffer);
		void PresentFrame();
		void ProcessResize(int width, int height);
		void CreateFramebuffers(const VkRenderPass& renderPass);
		void ClearFramebuffers();
		void InitializeBuffers();
		void InitializeDescriptorSets();
		
		//void updateComputeUniformBuffer(uint32_t currentImage);
		//void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
		//void drawRayTraced(VkCommandBuffer &r_CommandBuffer, uint32_t imageIndex);
	private:
		Settings m_Settings;
		bool m_FramebufferResized = false;
		uint32_t m_CurrentFrame = 0;
		uint32_t m_ImageIndex = 0;
		Assets::Scene* p_ActiveScene = nullptr;

		std::unique_ptr<class Window> m_Window;
		std::unique_ptr<class Instance> m_Instance;
		std::unique_ptr<class Surface> m_Surface;
		std::unique_ptr<class PhysicalDevice> m_PhysicalDevice;
		std::unique_ptr<class LogicalDevice> m_LogicalDevice;
		std::unique_ptr<class SwapChain> m_SwapChain;
		//std::unique_ptr<class GraphicsPipeline> m_TempRayTracerPipeline;
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
		std::unique_ptr<class CommandBuffer> m_CommandBuffers;
		std::unique_ptr<class Buffer> m_ComputeUniformBuffers;
		std::unique_ptr<class CommandBuffer> m_ComputeCommandBuffers;
		std::unique_ptr<class Buffer> m_ShaderStorageBuffers;
		std::unique_ptr<class DescriptorPool> m_DescriptorPool;
		std::map<std::string, std::unique_ptr<class GraphicsPipeline>> m_GraphicsPipelines;
		std::unique_ptr<std::map<std::string, std::unique_ptr<class Assets::Material>>> m_Materials;
		std::unique_ptr<std::map<std::string, std::unique_ptr<struct Assets::Texture>>> m_LoadedTextures;
		std::unique_ptr<class DescriptorSetLayout> m_ObjectGPUDataDescriptorSetLayout;
		std::unique_ptr<class DescriptorSetLayout> m_SceneGPUDataDescriptorSetLayout;
		std::unique_ptr<class DescriptorSetLayout> m_MaterialGPUDataDescriptorSetLayout;
		std::unique_ptr<class DescriptorSets> m_SceneGPUDataDescriptorSets;

		/*	Scene Buffer Layout
			[index obj1 | index obj2 | index obj3 | vertex obj1 | vertex obj2 | vertex obj3]
		*/
		std::unique_ptr<class Engine::Buffer> m_SceneGeometryBuffer;

		/* GPUDataBuffer Layout
		   [object data 1 | object data 2 | materials 1 | materials 2]	
		*/
		std::unique_ptr<class Engine::Buffer> m_GPUDataBuffer;

		const int OBJECT_BUFFER_INDEX = 0;
		const int MATERIAL_BUFFER_INDEX = 1;
		const int SCENE_BUFFER_INDEX = 2;
		const int INDEX_BUFFER_INDEX = 0;			// :) 
		const int VERTEX_BUFFER_INDEX = 1;
		const std::string DEFAULT_GRAPHICS_PIPELINE = "defaultPipeline";
	};
}
