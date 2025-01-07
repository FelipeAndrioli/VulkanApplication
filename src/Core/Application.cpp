#include "Application.h"

#include "BufferManager.h"
#include "ResourceManager.h"

Application::~Application() {
	m_UI.reset();
	
	m_GraphicsDevice->DestroyDescriptorPool();
	m_GraphicsDevice.reset();

	m_Input.reset();
	m_Window.reset();
}

void Application::InitializeResources(IScene& scene) {
	m_Window = std::make_unique<Window>(scene.settings);
	m_Input = std::make_unique<InputSystem::Input>();

	m_Window->OnKeyPress = std::bind(&InputSystem::Input::ProcessKey, m_Input.get(), std::placeholders::_1, std::placeholders::_2,
		std::placeholders::_3, std::placeholders::_4);
	m_Window->OnResize = std::bind(&Application::Resize, this, std::placeholders::_1, std::placeholders::_2);
	m_Window->OnMouseClick = std::bind(&InputSystem::Input::ProcessMouseClick, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	m_Window->OnCursorMove = std::bind(&InputSystem::Input::ProcessCursorMove, m_Input.get(), std::placeholders::_1, std::placeholders::_2);
	m_Window->OnCursorOnScreen = std::bind(&InputSystem::Input::ProcessCursorOnScreen, m_Input.get(), std::placeholders::_1);

	m_GraphicsDevice = std::make_unique<Graphics::GraphicsDevice>(*m_Window.get());
	Graphics::GetDevice() = m_GraphicsDevice.get();

	m_GraphicsDevice->CreateDescriptorPool();

	Graphics::InitializeRenderingImages(m_GraphicsDevice->GetSwapChainExtent().width, m_GraphicsDevice->GetSwapChainExtent().height);

	m_UI = std::make_unique<UI>(*m_Window->GetHandle(), m_GraphicsDevice->GetSwapChain().renderPass);
}

void Application::RunApplication(IScene& scene) {

	Timestep initStartTime = glfwGetTime();
	InitializeResources(scene);
	Timestep resourcesInitializedTime = glfwGetTime();
	InitializeApplication(scene);
	Timestep sceneInitializedTime = glfwGetTime();

	std::cout << "Application initialization time: " << resourcesInitializedTime.GetSeconds() - initStartTime.GetSeconds() << " seconds." << '\n';
	std::cout << "Scene initialization time: " << sceneInitializedTime.GetSeconds() - resourcesInitializedTime.GetSeconds() << " seconds." << '\n';
	std::cout << "Total initialization time: " << sceneInitializedTime.GetSeconds() - initStartTime.GetSeconds() << " seconds." << '\n';

	while (UpdateApplication(scene)) {
		m_Input->Update();
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
		
		m_GraphicsDevice->ResizeRenderPass(
			m_GraphicsDevice->GetSwapChain().swapChainExtent.width,
			m_GraphicsDevice->GetSwapChain().swapChainExtent.height,
			scene.renderPass);

		scene.Resize(m_Window->GetFramebufferSize().width, m_Window->GetFramebufferSize().height);
	}

	if (m_Vsync && timestep.GetSeconds() < (1 / 60.0f)) return true;

	if (m_Input->Keys[GLFW_KEY_I].IsPressed)
		scene.settings.uiEnabled = !scene.settings.uiEnabled;

	scene.Update(timestep.GetMilliseconds(), *m_Input.get());

	if (!m_GraphicsDevice->BeginFrame(m_GraphicsDevice->GetCurrentFrame()))
		return true;

	Graphics::Frame& frame = m_GraphicsDevice->GetCurrentFrame();

	scene.RenderScene(m_GraphicsDevice->GetCurrentFrameIndex(), frame.commandBuffer);

	// SwapChain Render Pass
	{
		m_GraphicsDevice->BeginRenderPass(m_GraphicsDevice->GetSwapChain().renderPass, frame.commandBuffer);
		if (m_UI && scene.settings.uiEnabled) {
			m_UI->BeginFrame();
			RenderCoreUI();
			scene.RenderUI();
			m_UI->EndFrame(frame.commandBuffer);
		}
		m_GraphicsDevice->EndRenderPass(frame.commandBuffer);
	}

	m_GraphicsDevice->EndFrame(frame);

	// TODO: find an alternative solution to fix image artifacts since this barrier is quite expensive.

	/*
		This barrier ensures the swapchain layout transition, there's no validation layer error if removed, but
		some artifacts appears in the image while moving. This barrier costs about 1ms.
	*/
	m_GraphicsDevice->TransitionImageLayout(
		m_GraphicsDevice->GetSwapChain().swapChainImages[m_GraphicsDevice->GetSwapChain().imageIndex],
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);

	m_GraphicsDevice->PresentFrame(frame);

	m_LastFrameTime = m_CurrentFrameTime;

	m_Milliseconds = timestep.GetMilliseconds();
	m_FramesPerSecond = static_cast<float>(1 / timestep.GetSeconds());

	return !scene.IsDone(*m_Input.get());
}

void Application::InitializeApplication(IScene& scene) {
	scene.StartUp();
}

void Application::TerminateApplication(IScene& scene) {
	Graphics::ShutdownRenderingImages();

	scene.CleanUp();

	ResourceManager::Get()->Destroy();
}

void Application::Resize(int width, int height) {
	std::cout << "width - " << width << " height - " << height << '\n';

	m_GraphicsDevice->RecreateSwapChain(*m_Window.get());

	Graphics::ResizeDisplayDependentImages(
		m_GraphicsDevice->GetSwapChainExtent().width, 
		m_GraphicsDevice->GetSwapChainExtent().height);

	m_ResizeApplication = true;
}
