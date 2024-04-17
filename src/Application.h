#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>

#include <glm/glm.hpp>

#include "Settings.h"
#include "Vulkan.h"

namespace Assets {
	class Scene;
	class Object;
	struct Material;

	struct Texture;
}

namespace Engine {
	namespace InputSystem {
		class Input;
	}

	struct Settings;

	struct ObjectGPUData {
		glm::vec4 extra[12];
		glm::mat4 model = glm::mat4(1.0f);
	};

	struct SceneGPUData {
		float time = 0.0f;
		float extra_s_1 = 0.0f;
		float extra_s_2 = 0.0f;
		float extra_s_3 = 0.0f;
		glm::vec4 extra[7];
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
		void Draw();
		void DrawFrame(const VkCommandBuffer& commandBuffer);
		void RenderScene(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline, const std::vector<Assets::Object*>& objects);
		void DrawUI();
		void ProcessResize(int width, int height);
		void InitializeBuffers();
		void InitializeDescriptors();
		void CreatePipelineLayouts();
		void CreateGraphicsPipelines();
		void BeginRenderPass(const VkRenderPass& renderPass, VkCommandBuffer& commandBuffer);
		void EndRenderPass(VkCommandBuffer& commandBuffer);
	private:
		Settings m_Settings;
		bool m_FramebufferResized = false;
		uint32_t m_CurrentFrame = 0;
		uint32_t m_ImageIndex = 0;
		Assets::Scene* p_ActiveScene = nullptr;

		std::unique_ptr<class Window> m_Window;
		std::unique_ptr<class VulkanEngine> m_VulkanEngine;

		std::unique_ptr<class InputSystem::Input> m_Input;
		std::unique_ptr<class Buffer> m_ComputeUniformBuffers;
		std::unique_ptr<class CommandBuffer> m_ComputeCommandBuffers;
		std::unique_ptr<class Buffer> m_ShaderStorageBuffers;
		std::unique_ptr<class DescriptorPool> m_DescriptorPool;

		std::unique_ptr<class PipelineLayout> m_MainPipelineLayout;

		std::unique_ptr<class GraphicsPipeline> m_TexturedPipeline;
		std::unique_ptr<class GraphicsPipeline> m_WireframePipeline;
		std::unique_ptr<class GraphicsPipeline> m_ColoredPipeline;

		std::vector<Assets::Material> m_Materials;
		std::vector<Assets::Texture> m_LoadedTextures;

		std::unique_ptr<class DescriptorSetLayout> m_ObjectGPUDataDescriptorSetLayout;
		std::unique_ptr<class DescriptorSetLayout> m_GlobalDescriptorSetLayout;

		std::unique_ptr<class DescriptorSets> m_GlobalDescriptorSets;

		/*	Scene Buffer Layout
			[index obj1 | index obj2 | index obj3 | vertex obj1 | vertex obj2 | vertex obj3]
		*/
		std::unique_ptr<class Engine::Buffer> m_SceneGeometryBuffer;

		/* GPUDataBuffer Layout
		   [object 1 | object 2 | materials 1 | materials 2 | scene data]	
		*/
		std::unique_ptr<class Engine::Buffer> m_GPUDataBuffer;

		SceneGPUData m_SceneGPUData;

		static const int OBJECT_BUFFER_INDEX = 0;
		static const int MATERIAL_BUFFER_INDEX = 1;
		static const int SCENE_BUFFER_INDEX = 2;
		static const int INDEX_BUFFER_INDEX = 0;			// :) 
		static const int VERTEX_BUFFER_INDEX = 1;
		const std::string DEFAULT_GRAPHICS_PIPELINE = "defaultPipeline";
	};
}
