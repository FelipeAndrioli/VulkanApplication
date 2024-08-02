#include "ApplicationCore.h"

namespace Engine {

	ApplicationCore::ApplicationCore(Settings& settings) {

		m_Window = std::make_unique<Window>(settings);
		m_Input = std::make_unique<InputSystem::Input>();

		m_Window->OnKeyPress = std::bind(&InputSystem::Input::ProcessKey, m_Input.get(), std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4);
		m_Window->OnResize = std::bind(&ApplicationCore::Resize, this, std::placeholders::_1, std::placeholders::_2);
		m_Window->OnMouseClick = std::bind(&InputSystem::Input::ProcessMouseClick, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		m_Window->OnCursorMove = std::bind(&InputSystem::Input::ProcessCursorMove, m_Input.get(), std::placeholders::_1, std::placeholders::_2);
		m_Window->OnCursorOnScreen = std::bind(&InputSystem::Input::ProcessCursorOnScreen, m_Input.get(), std::placeholders::_1);

		m_GraphicsDevice = std::make_unique<Engine::Graphics::GraphicsDevice>(*m_Window.get());
		Graphics::GetDevice() = m_GraphicsDevice.get();

		bool success = m_GraphicsDevice->CreateSwapChain(*m_Window.get(), m_SwapChain);

		assert(success);

		m_GraphicsDevice->CreateRenderTarget(m_RenderTarget, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height, m_SwapChain.swapChainImageFormat);
		m_GraphicsDevice->CreateDepthBuffer(m_DepthBuffer, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height);

		m_Framebuffers.resize(m_SwapChain.swapChainImageViews.size());

		for (int i = 0; i < m_SwapChain.swapChainImageViews.size(); i++) {
			std::vector<VkImageView> framebufferAttachments = { m_RenderTarget.ImageView, m_DepthBuffer.ImageView, m_SwapChain.swapChainImageViews[i] };
			m_GraphicsDevice->CreateFramebuffer(m_GraphicsDevice->defaultRenderPass, framebufferAttachments, m_SwapChain.swapChainExtent, m_Framebuffers[i]);
		}

		m_UI = std::make_unique<Engine::UI>(*m_Window->GetHandle());
	}

	ApplicationCore::~ApplicationCore() {
		m_UI.reset();
		
		m_GraphicsDevice->DestroyFramebuffer(m_Framebuffers);
		m_GraphicsDevice->DestroyImage(m_DepthBuffer);
		m_GraphicsDevice->DestroyImage(m_RenderTarget);
		m_GraphicsDevice->DestroySwapChain(m_SwapChain);
		m_GraphicsDevice.reset();

		m_Input.reset();
		m_Window.reset();
	}

	void ApplicationCore::RunApplication(IScene& scene) {
		InitializeApplication(scene);

		while (UpdateApplication(scene)) {
			glfwPollEvents();
		}

		m_GraphicsDevice->WaitIdle();
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

		VkCommandBuffer* commandBuffer = m_GraphicsDevice->BeginFrame(m_SwapChain);

		if (commandBuffer == nullptr)
			return true;

		//m_VulkanEngine->BeginRenderPass(m_VulkanEngine->GetDefaultRenderPass().GetHandle(), *commandBuffer, m_VulkanEngine->GetFramebuffer(m_ImageIndex));
		//m_GraphicsDevice->BeginRenderPass(m_DefaultRenderPass, *commandBuffer, m_SwapChain.swapChainExtent);
		//m_GraphicsDevice->BeginRenderPass(*commandBuffer, m_SwapChain.swapChainExtent, m_SwapChain.imageIndex, m_Framebuffers[m_SwapChain.imageIndex]);
		m_GraphicsDevice->BeginRenderPass(m_GraphicsDevice->defaultRenderPass, *commandBuffer, m_SwapChain.swapChainExtent, m_SwapChain.imageIndex, m_Framebuffers[m_SwapChain.imageIndex]);

		if (m_UI) {
			m_UI->BeginFrame();
		}

		scene.RenderScene(m_CurrentFrame, *commandBuffer);
		RenderCoreUI();
		scene.RenderUI();

		if (m_UI) {
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

	void ApplicationCore::InitializeApplication(IScene& scene) {
		scene.StartUp(*m_GraphicsDevice.get());
	}

	void ApplicationCore::TerminateApplication(IScene& scene) {
		scene.CleanUp(*m_GraphicsDevice.get());

		m_GraphicsDevice->DestroyImage(m_RenderTarget);
		m_GraphicsDevice->DestroyImage(m_DepthBuffer);

		m_Input.reset();
		m_Window.reset();
		m_GraphicsDevice.reset();
	}

	void ApplicationCore::Resize(int width, int height) {
		std::cout << "width - " << width << " height - " << height << '\n';

		m_GraphicsDevice->RecreateSwapChain(*m_Window.get(), m_SwapChain);
		m_GraphicsDevice->ResizeImage(m_RenderTarget, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height);
		m_GraphicsDevice->ResizeImage(m_DepthBuffer, m_SwapChain.swapChainExtent.width, m_SwapChain.swapChainExtent.height);

		// this might cause problem, need to test
		// do we even need to recreate the render pass?
		//m_GraphicsDevice->RecreateDefaultRenderPass(m_DefaultRenderPass, m_SwapChain);

		m_GraphicsDevice->DestroyFramebuffer(m_Framebuffers);

		for (int i = 0; i < m_SwapChain.swapChainImageViews.size(); i++) {
			std::vector<VkImageView> framebufferAttachments = { m_RenderTarget.ImageView, m_DepthBuffer.ImageView, m_SwapChain.swapChainImageViews[i] };
			m_GraphicsDevice->CreateFramebuffer(m_GraphicsDevice->defaultRenderPass, framebufferAttachments, m_SwapChain.swapChainExtent, m_Framebuffers[i]);
		}

		m_GraphicsDevice->RecreateCommandBuffers();

		m_ResizeApplication = true;
	}
}