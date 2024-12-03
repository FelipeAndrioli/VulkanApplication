#pragma once

#include <glm.hpp>

#include "VulkanHeader.h"
#include "Window.h"
#include "UI.h"
#include "Settings.h"

#include "GraphicsDevice.h"
#include "Graphics.h"

#include "../Input/Input.h"

#define RUN_APPLICATION(class_name)		\
	int main(int argc, char* argv[]) {	\
		Application app;				\
		class_name scene;				\
		app.RunApplication(scene);		\
										\
		return 0;						\
	}									

class Application {
public:

	class IScene {
	public:
		virtual void StartUp() = 0;
		virtual void CleanUp() = 0;
		
		virtual bool IsDone(InputSystem::Input& input) {
			return input.Keys[GLFW_KEY_ESCAPE].IsPressed;
		}

		virtual void Update(float d, InputSystem::Input& input) = 0;
		virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) = 0;
		virtual void RenderUI() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

	public:
		Settings settings = {};

		Graphics::RenderPass renderPass = {};
	};

	Application() {};
	~Application();

	void RunApplication(IScene& scene);
	void RenderCoreUI();
	bool UpdateApplication(IScene& scene);
	void InitializeApplication(IScene& scene);
	void TerminateApplication(IScene& scene);
private:
	void InitializeResources(IScene& scene);
	void Resize(int width, int height);
private:
	float m_CurrentFrameTime = 0.0f;
	float m_LastFrameTime = 0.0f;
	float m_Milliseconds = 0.0f;
	float m_FramesPerSecond = 0.0f;

	bool m_Vsync = false;
	bool m_ResizeApplication = false;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::PipelineState m_Pso = {};
	Graphics::PipelineStateDescription m_PsoDesc = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_Set[Graphics::FRAMES_IN_FLIGHT];

	Graphics::InputLayout m_InputLayout = {
		.pushConstants = {},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		}
	};

	std::unique_ptr<Window> m_Window;
	std::unique_ptr<InputSystem::Input> m_Input;

	std::unique_ptr<Graphics::GraphicsDevice> m_GraphicsDevice;
	std::unique_ptr<UI> m_UI;
};
