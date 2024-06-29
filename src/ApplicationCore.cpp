#include "ApplicationCore.h"

namespace Engine {

	ApplicationCore::ApplicationCore(Settings &settings) {

		m_Window.reset(new class Window(settings));
		m_Input.reset(new class InputSystem::Input());

		//m_Window->Render = std::bind(&Application::Draw, this);
		m_Window->OnKeyPress = std::bind(&InputSystem::Input::ProcessKey, m_Input.get(), std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4);
		m_Window->OnResize = std::bind(&ApplicationCore::Resize, this, std::placeholders::_1, std::placeholders::_2);
		m_Window->OnMouseClick = std::bind(&InputSystem::Input::ProcessMouseClick, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		m_Window->OnCursorMove = std::bind(&InputSystem::Input::ProcessCursorMove, m_Input.get(), std::placeholders::_1, std::placeholders::_2);
		m_Window->OnCursorOnScreen = std::bind(&InputSystem::Input::ProcessCursorOnScreen, m_Input.get(), std::placeholders::_1);

		//m_VulkanEngine = std::make_unique<class VulkanEngine>(*m_Window.get());
		graphicsDevice = std::make_unique<Engine::Graphics::GraphicsDevice>(*m_Window.get());
		bool success = graphicsDevice->CreateSwapChain(*m_Window.get(), swapChain);

		assert(success);
	}
	
	ApplicationCore::~ApplicationCore() {
	
	}

	void ApplicationCore::RunApplication(IScene& scene) {
		InitializeApplication(scene);

		while (UpdateApplication(scene)) {
			glfwPollEvents();
		}

		graphicsDevice->WaitIdle();
		TerminateApplication(scene);
	}

	void ApplicationCore::RenderCoreUI() {
		ImGui::Begin("Settings");
		ImGui::SeparatorText("Application Core");
		ImGui::Text("Last Frame: %f ms", m_Milliseconds);
		ImGui::Text("Framerate: %.1f fps", m_FramesPerSecond);
		ImGui::Checkbox("Limit Framerate", &m_Vsync);
	}

	bool ApplicationCore::UpdateApplication(IScene& scene) {
		m_CurrentFrameTime = (float)glfwGetTime();
		Timestep timestep = m_CurrentFrameTime - m_LastFrameTime;

		if (m_ResizeApplication) {
			m_ResizeApplication = false;
			scene.Resize(m_Window->GetFramebufferSize().width, m_Window->GetFramebufferSize().height);
		}

		if (m_Vsync && timestep.GetSeconds() < (1 / 60.0f)) return true;
		
		scene.Update(timestep.GetMilliseconds(), *m_Input.get());

		VkCommandBuffer* commandBuffer = m_VulkanEngine->BeginFrame(m_CurrentFrame, m_ImageIndex);

		if (commandBuffer == nullptr)
			return true;

		m_VulkanEngine->BeginRenderPass(m_VulkanEngine->GetDefaultRenderPass().GetHandle(), *commandBuffer, m_VulkanEngine->GetFramebuffer(m_ImageIndex));
		m_VulkanEngine->BeginUIFrame();

		scene.RenderScene(m_CurrentFrame, *commandBuffer);
		RenderCoreUI();
		scene.RenderUI();

		m_VulkanEngine->EndUIFrame(*commandBuffer);
		m_VulkanEngine->EndRenderPass(*commandBuffer);
		m_VulkanEngine->EndFrame(*commandBuffer, m_CurrentFrame, m_ImageIndex);
		m_VulkanEngine->PresentFrame(m_CurrentFrame, m_ImageIndex);

		m_LastFrameTime = m_CurrentFrameTime;

		m_Milliseconds = timestep.GetMilliseconds();
		m_FramesPerSecond = static_cast<float>(1 / timestep.GetSeconds());

		return !scene.IsDone(*m_Input.get());
	}

	void ApplicationCore::InitializeApplication(IScene& scene) {
		scene.StartUp(*m_VulkanEngine.get());
	}

	void ApplicationCore::TerminateApplication(IScene& scene) {
		scene.CleanUp();

		m_Input.reset();
		m_Window.reset();
		m_VulkanEngine.reset();
	}

	void ApplicationCore::Resize(int width, int height) {
		std::cout << "width - " << width << " height - " << height << '\n';
		m_VulkanEngine->Resize();
		m_ResizeApplication = true;
	}
}