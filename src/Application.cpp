#include "Application.h"

namespace Engine {

	Application::~Application() {
		m_UI.reset();
		
		m_GraphicsDevice->DestroyFramebuffer(m_Framebuffers);
		m_Framebuffers.clear();

		m_GraphicsDevice->DestroyImage(m_DepthBuffer);
		m_GraphicsDevice->DestroyImage(m_RenderTarget);
		m_GraphicsDevice->DestroySwapChain(m_SwapChain);
		m_GraphicsDevice->DestroyDescriptorPool();
		m_GraphicsDevice.reset();

		m_Input.reset();
		m_Window.reset();
	}

	void Application::InitializeResources(Settings& settings) {
		m_Window = std::make_unique<Window>(settings);
		m_Input = std::make_unique<InputSystem::Input>();

		m_Window->OnKeyPress = std::bind(&InputSystem::Input::ProcessKey, m_Input.get(), std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4);
		m_Window->OnResize = std::bind(&Application::Resize, this, std::placeholders::_1, std::placeholders::_2);
		m_Window->OnMouseClick = std::bind(&InputSystem::Input::ProcessMouseClick, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		m_Window->OnCursorMove = std::bind(&InputSystem::Input::ProcessCursorMove, m_Input.get(), std::placeholders::_1, std::placeholders::_2);
		m_Window->OnCursorOnScreen = std::bind(&InputSystem::Input::ProcessCursorOnScreen, m_Input.get(), std::placeholders::_1);

		m_GraphicsDevice = std::make_unique<Engine::Graphics::GraphicsDevice>(*m_Window.get());
		Graphics::GetDevice() = m_GraphicsDevice.get();

		bool success = m_GraphicsDevice->CreateSwapChain(*m_Window.get(), m_SwapChain);

		assert(success);

		m_GraphicsDevice->CreateDescriptorPool();
		m_GraphicsDevice->CreateRenderTarget(m_RenderTarget, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height, m_SwapChain.swapChainImageFormat);
		m_GraphicsDevice->CreateDepthBuffer(m_DepthBuffer, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height);

		m_Framebuffers.resize(m_SwapChain.swapChainImageViews.size());

		for (int i = 0; i < m_SwapChain.swapChainImageViews.size(); i++) {
			std::vector<VkImageView> framebufferAttachments = { m_RenderTarget.ImageView, m_DepthBuffer.ImageView, m_SwapChain.swapChainImageViews[i] };
			m_GraphicsDevice->CreateFramebuffer(m_GraphicsDevice->defaultRenderPass, framebufferAttachments, m_SwapChain.swapChainExtent, m_Framebuffers[i]);
		}

		m_UI = std::make_unique<Engine::UI>(*m_Window->GetHandle());
	}

	void Application::RunApplication(IScene& scene) {
		InitializeResources(scene.settings);
		InitializeApplication(scene);

		while (UpdateApplication(scene)) {
			glfwPollEvents();
		}

		m_GraphicsDevice->WaitIdle();
		TerminateApplication(scene);
	}

	void Application::RenderCoreUI() {
		ImGui::Begin("Settings");
		ImGui::SeparatorText("Application");
		ImGui::Text("Last Frame: %f ms", m_Milliseconds);
		ImGui::Text("Framerate: %.1f fps", m_FramesPerSecond);
		ImGui::Checkbox("Limit Framerate", &m_Vsync);
	}

	bool Application::UpdateApplication(IScene& scene) {
		m_CurrentFrameTime = (float)glfwGetTime();
		Timestep timestep = m_CurrentFrameTime - m_LastFrameTime;

		if (m_ResizeApplication) {
			m_ResizeApplication = false;
			scene.Resize(m_Window->GetFramebufferSize().width, m_Window->GetFramebufferSize().height);
		}

		if (m_Vsync && timestep.GetSeconds() < (1 / 60.0f)) return true;
		
		scene.Update(timestep.GetMilliseconds(), *m_Input.get());

		VkCommandBuffer* commandBuffer = m_GraphicsDevice->BeginFrame(m_SwapChain);

		if (commandBuffer == nullptr)
			return true;

		m_GraphicsDevice->BeginRenderPass(m_GraphicsDevice->defaultRenderPass, *commandBuffer, m_SwapChain.swapChainExtent, m_SwapChain.imageIndex, m_Framebuffers[m_SwapChain.imageIndex]);

		scene.RenderScene(m_CurrentFrame, *commandBuffer);

		if (m_UI) {
			m_UI->BeginFrame();
			RenderCoreUI();
			scene.RenderUI();
			m_UI->EndFrame(*commandBuffer);
		}

		m_GraphicsDevice->EndRenderPass(*commandBuffer);
		m_GraphicsDevice->EndFrame(*commandBuffer, m_SwapChain);
		m_GraphicsDevice->PresentFrame(m_SwapChain);

		m_LastFrameTime = m_CurrentFrameTime;

		m_Milliseconds = timestep.GetMilliseconds();
		m_FramesPerSecond = static_cast<float>(1 / timestep.GetSeconds());

		return !scene.IsDone(*m_Input.get());
	}

	void Application::InitializeApplication(IScene& scene) {
		scene.StartUp();
	}

	void Application::TerminateApplication(IScene& scene) {
		scene.CleanUp();
	}

	void Application::Resize(int width, int height) {
		std::cout << "width - " << width << " height - " << height << '\n';

		m_GraphicsDevice->RecreateSwapChain(*m_Window.get(), m_SwapChain);

		m_GraphicsDevice->ResizeImage(m_RenderTarget, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height);
		m_GraphicsDevice->ResizeImage(m_DepthBuffer, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height);

		m_GraphicsDevice->DestroyFramebuffer(m_Framebuffers);

		for (int i = 0; i < m_SwapChain.swapChainImageViews.size(); i++) {
			std::vector<VkImageView> framebufferAttachments = { m_RenderTarget.ImageView, m_DepthBuffer.ImageView, m_SwapChain.swapChainImageViews[i] };
			m_GraphicsDevice->CreateFramebuffer(m_GraphicsDevice->defaultRenderPass, framebufferAttachments, m_SwapChain.swapChainExtent, m_Framebuffers[i]);
		}

		m_GraphicsDevice->RecreateCommandBuffers();

		m_ResizeApplication = true;
	}
}