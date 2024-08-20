#pragma once

#include <glm.hpp>

#include "VulkanHeader.h"
#include "Window.h"
#include "UI.h"
#include "Settings.h"

#include "GraphicsDevice.h"
#include "Graphics.h"

#include "Input/Input.h"

#define RUN_APPLICATION(class_name)		\
	int main(int argc, char* argv[]) {	\
		Engine::Application app;		\
		class_name scene;				\
		app.RunApplication(scene);		\
										\
		return 0;						\
	}									

namespace Engine {
	class Application {
	public:

		class IScene {
		public:
			virtual void StartUp() = 0;
			virtual void CleanUp() = 0;
			virtual bool IsDone(Engine::InputSystem::Input& input) = 0;
			virtual void Update(float d, Engine::InputSystem::Input& input) = 0;
			virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) = 0;
			virtual void RenderUI() = 0;
			virtual void Resize(uint32_t width, uint32_t height) = 0;

		public:
			Settings settings = {};
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
			glm::vec4 extra[12] = {};
			glm::mat4 model = glm::mat4(1.0f);
		};

		Application() {};
		~Application();

		void RunApplication(IScene& scene);
		void RenderCoreUI();
		bool UpdateApplication(IScene& scene);
		void InitializeApplication(IScene& scene);
		void TerminateApplication(IScene& scene);
	private:
		Engine::Graphics::Frame& GetCurrentFrame();
		Engine::Graphics::Frame& GetLastFrame();

		void InitializeResources(Settings& settings);
		void Resize(int width, int height);
	private:
		float m_CurrentFrameTime = 0.0f;
		float m_LastFrameTime = 0.0f;
		float m_Milliseconds = 0.0f;
		float m_FramesPerSecond = 0.0f;
	
		bool m_Vsync = false;
		bool m_ResizeApplication = false;

		SceneGPUData m_SceneGPUData = {};

		std::unique_ptr<Window> m_Window;
		std::unique_ptr<InputSystem::Input> m_Input;

		Engine::Graphics::SwapChain m_SwapChain;
		Engine::Graphics::Frame m_Frames[Engine::Graphics::FRAMES_IN_FLIGHT];

		std::unique_ptr<Engine::Graphics::GraphicsDevice> m_GraphicsDevice;

		Graphics::GPUImage m_RenderTarget = {};
		Graphics::GPUImage m_DepthBuffer = {};

		std::vector<VkFramebuffer> m_Framebuffers;

		std::unique_ptr<Engine::UI> m_UI;
	};
}