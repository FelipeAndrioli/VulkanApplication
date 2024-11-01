#include "Application.h"

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

	Graphics::RenderPassDesc desc = {};
	desc.scissor.offset = { 0, 0 };
	desc.scissor.extent = m_GraphicsDevice->GetSwapChain().swapChainExtent;
	desc.viewport.x = 0.0f;
	desc.viewport.y = 0.0f;
	desc.viewport.width = static_cast<float>(m_GraphicsDevice->GetSwapChain().swapChainExtent.width);
	desc.viewport.height = static_cast<float>(m_GraphicsDevice->GetSwapChain().swapChainExtent.height);
	desc.viewport.minDepth = 0.0f;
	desc.viewport.maxDepth = 1.0f;
	desc.clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	desc.clearValues[1].depthStencil = { 1.0f, 0 };
	desc.clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	desc.sampleCount = m_GraphicsDevice->m_MsaaSamples;
	desc.flags = Graphics::RenderPass::tColorAttachment | Graphics::RenderPass::tDepthAttachment | Graphics::RenderPass::tColorResolveAttachment;

	m_GraphicsDevice->CreateRenderPass(desc, scene.renderPass);

	m_UI = std::make_unique<UI>(*m_Window->GetHandle(), scene.renderPass);
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
	
	scene.Update(timestep.GetMilliseconds(), *m_Input.get());

	if (!m_GraphicsDevice->BeginFrame(m_GraphicsDevice->GetCurrentFrame()))
		return true;

	Graphics::Frame& frame = m_GraphicsDevice->GetCurrentFrame();
	
	m_GraphicsDevice->BeginRenderPass(scene.renderPass, frame.commandBuffer);
	
	scene.RenderScene(m_GraphicsDevice->GetCurrentFrameIndex(), frame.commandBuffer);
	
	if (m_UI) {
		m_UI->BeginFrame();
		RenderCoreUI();
		scene.RenderUI();
		m_UI->EndFrame(frame.commandBuffer);
	}

	m_GraphicsDevice->EndRenderPass(frame.commandBuffer);

	m_GraphicsDevice->EndFrame(frame);
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
	m_GraphicsDevice->DestroyRenderPass(scene.renderPass);
	scene.CleanUp();
}

void Application::Resize(int width, int height) {
	std::cout << "width - " << width << " height - " << height << '\n';

	m_GraphicsDevice->RecreateSwapChain(*m_Window.get());

	m_ResizeApplication = true;
}
