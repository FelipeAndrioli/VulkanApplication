#pragma once

#include <glm.hpp>

#include "VulkanHeader.h"
#include "Vulkan.h"
#include "Window.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "DescriptorSets.h"
#include "Buffer.h"

#include "Input/Input.h"
#include "Assets/Object.h"

namespace Engine {
	class ApplicationCore {
	public:

	class IScene {
	public:
		virtual void StartUp(Engine::VulkanEngine& vulkanEngine) = 0;
		virtual void CleanUp() = 0;
		virtual bool IsDone(InputSystem::Input& input);
		virtual void Update(float d, Engine::InputSystem::Input& input) = 0;
		virtual void RenderScene() = 0;
		virtual void RenderUI() = 0;

		bool IsDone(InputSystem::Input& input) {
			return input.Keys[GLFW_KEY_ESCAPE].IsPressed;
		}
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

	struct ObjectGPUData {
		glm::vec4 extra[12];
		glm::mat4 model = glm::mat4(1.0f);
	};

		ApplicationCore(const Settings &settings);
		~ApplicationCore();

		void RunApplication(IScene& scene);
		bool UpdateApplication(IScene& scene);
		void InitializeApplication(IScene& scene);
		void TerminateApplication(IScene& scene);

		void RenderSkybox(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline);
		void RenderModel(const VkCommandBuffer& commandBuffer, const Assets::Object& object);
		void RenderScene(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline, const std::vector<Assets::Object*>& objects);

	private:
		void Resize(int width, int height);
	private:
		float m_CurrentFrameTime = 0.0;
		float m_LastFrameTime = 0.0;

		uint32_t m_CurrentFrame = 0;

		Settings m_Settings;
		SceneGPUData m_SceneGPUData;
		std::unique_ptr<Window> m_Window;
		std::unique_ptr<InputSystem::Input> m_Input;
		std::unique_ptr<Engine::VulkanEngine> m_VulkanEngine;
	};
}