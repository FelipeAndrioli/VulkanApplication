#include "Application.h"

#include "BufferManager.h"
#include "ResourceManager.h"
#include "RenderTarget.h"
#include "Profiler.h"

Application::~Application() {
	static Profiler* profiler = Profiler::Get();
	profiler->Destroy();

	m_UI.reset();
	
	m_GraphicsDevice->DestroyDescriptorPool();
	m_GraphicsDevice.reset();

	m_Input.reset();
	m_Window.reset();
}

void Application::InitializeResources(IScene& scene) {
	m_Window = std::make_unique<Window>(scene.settings);
	m_Input = std::make_unique<InputSystem::Input>();

	m_Window->OnKeyPress		= std::bind(&InputSystem::Input::ProcessKey, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	m_Window->OnResize			= std::bind(&Application::Resize, this, std::placeholders::_1, std::placeholders::_2);
	m_Window->OnMouseClick		= std::bind(&InputSystem::Input::ProcessMouseClick, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	m_Window->OnCursorMove		= std::bind(&InputSystem::Input::ProcessCursorMove, m_Input.get(), std::placeholders::_1, std::placeholders::_2);
	m_Window->OnCursorOnScreen	= std::bind(&InputSystem::Input::ProcessCursorOnScreen, m_Input.get(), std::placeholders::_1);

	m_GraphicsDevice = std::make_unique<Graphics::GraphicsDevice>(*m_Window.get());
	Graphics::GetDevice() = m_GraphicsDevice.get();

	m_GraphicsDevice->CreateDescriptorPool();
	m_GraphicsDevice->CreateSwapChainRenderTarget();

	if (scene.settings.uiEnabled)
		m_UI = std::make_unique<UI>(*m_Window->GetHandle(), m_GraphicsDevice->GetSwapChain().RenderTarget->GetRenderPass());
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

	if (ImGui::TreeNode("Profiler")) {

		ImGui::Separator();

		std::unordered_map<std::string, std::vector<Profiler::Item>>::iterator it;

		static Profiler* profiler = Profiler::Get();

		for (it = profiler->Items.begin(); it != profiler->Items.end(); it++) {
			if (ImGui::TreeNode(it->first.c_str())) {

				ImGui::Separator();

				std::vector<Profiler::Item>& profilerItems = it->second;

				for (size_t ProfilerItemIndex = 0; ProfilerItemIndex < profilerItems.size(); ++ProfilerItemIndex) {
					Profiler::Item& Item = profilerItems[ProfilerItemIndex];

					const std::string UnitTypeString = Item.UnitType == Profiler::UnitType::MS ? "ms" : "us";

					std::string ProfilerMessage = Item.FunctionId + ": " + std::to_string(Item.Duration) + UnitTypeString;

					ImGui::Text(ProfilerMessage.c_str());
				}

				ImGui::TreePop();
			}
		}
		
		ImGui::TreePop();
	}
}

bool Application::UpdateApplication(IScene& scene) {
	m_CurrentFrameTime = (float)glfwGetTime();
	Timestep timestep = m_CurrentFrameTime - m_LastFrameTime;

	if (m_ResizeApplication) {
		m_ResizeApplication = false;
		
		scene.Resize(m_Window->GetFramebufferSize().width, m_Window->GetFramebufferSize().height);
	}

	if (m_Vsync && timestep.GetSeconds() < (1 / 60.0f)) return true;

	if (m_Input->Keys[GLFW_KEY_I].IsPressed)
		scene.settings.uiEnabled = !scene.settings.uiEnabled;

	{
		SCOPED_PROFILER_US("Application::Update");
		scene.Update(m_CurrentFrameTime, timestep.GetMilliseconds(), *m_Input.get());
	}

	if (!m_GraphicsDevice->BeginFrame(m_GraphicsDevice->GetCurrentFrame()))
		return true;

	Graphics::Frame& frame = m_GraphicsDevice->GetCurrentFrame();

	{
		SCOPED_PROFILER_US("Application::RenderScene");
		scene.RenderScene(m_GraphicsDevice->GetCurrentFrameIndex(), frame.commandBuffer);
	}

	// SwapChain Render Pass
	{
		SCOPED_PROFILER_US("Application::SwapChain Pass");
		m_GraphicsDevice->GetSwapChain().RenderTarget->Begin(frame.commandBuffer);
		if (m_UI && scene.settings.uiEnabled) {
			SCOPED_PROFILER_US("Application::UI");
			m_UI->BeginFrame();
			RenderCoreUI();
			scene.RenderUI();
			m_UI->EndFrame(frame.commandBuffer);
		}
		m_GraphicsDevice->GetSwapChain().RenderTarget->End(frame.commandBuffer);
	}

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
	scene.CleanUp();
	
	ResourceManager::Get()->Destroy();
}

void Application::Resize(int width, int height) {
	std::cout << "width - " << width << " height - " << height << '\n';

	m_GraphicsDevice->RecreateSwapChain(*m_Window.get());

	m_ResizeApplication = true;
}

/*
	Known Issues:
		- When resizing with debug render passes enabled, if screen becomes too small the 
		  validation layer will complain about the destination image pRegion exceeding 
		  the destination image dimensions.
*/
