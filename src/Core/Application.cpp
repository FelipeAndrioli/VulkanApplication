#include "Application.h"

#include "RenderPassManager.h"
#include "BufferManager.h"
#include "PostEffects/PostEffects.h"

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
	Graphics::InitializeStaticRenderPasses(m_GraphicsDevice->GetSwapChainExtent().width, m_GraphicsDevice->GetSwapChainExtent().height);


#ifdef RUNTIME_SHADER_COMPILATION
	m_GraphicsDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Assets/Shaders/quad.vert");
	m_GraphicsDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Assets/Shaders/present.frag");
#else
	m_GraphicsDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "./Shaders/quad_vert.spv");
	m_GraphicsDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "./Shaders/present_frag.spv");
#endif

	m_PsoDesc.Name = "Present Pipeline";
	m_PsoDesc.vertexShader = &m_VertexShader;
	m_PsoDesc.fragmentShader = &m_FragShader;
	m_PsoDesc.psoInputLayout.push_back(m_InputLayout);
	m_PsoDesc.cullMode = VK_CULL_MODE_FRONT_BIT;
	m_PsoDesc.noVertex = true;

	m_GraphicsDevice->CreateDescriptorSetLayout(m_SetLayout, m_InputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		m_GraphicsDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		m_GraphicsDevice->WriteDescriptor(m_InputLayout.bindings[0], m_Set[i], Graphics::g_PostEffects);
	}

	m_GraphicsDevice->CreatePipelineState(m_PsoDesc, m_Pso, Graphics::g_FinalRenderPass);
	
	m_UI = std::make_unique<UI>(*m_Window->GetHandle(), Graphics::g_FinalRenderPass);
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

	PostEffects::Initialize();

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

	PostEffects::RenderUI();
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

	// Color Render Pass
	{
		m_GraphicsDevice->BeginRenderPass(Graphics::g_ColorRenderPass, frame.commandBuffer);
		scene.RenderScene(m_GraphicsDevice->GetCurrentFrameIndex(), frame.commandBuffer);
		m_GraphicsDevice->EndRenderPass(frame.commandBuffer);
	}

	// Post Effects Render Pass
	{
		m_GraphicsDevice->BeginRenderPass(Graphics::g_PostEffectsRenderPass, frame.commandBuffer);
		PostEffects::Render(frame.commandBuffer);
		m_GraphicsDevice->EndRenderPass(frame.commandBuffer);
	}

	// Present/Final Render Pass
	{
		m_GraphicsDevice->BeginRenderPass(Graphics::g_FinalRenderPass, frame.commandBuffer);

		if (!scene.settings.postEffectsEnabled && !PostEffects::Rendered) {
			m_GraphicsDevice->WriteDescriptor(m_InputLayout.bindings[0], m_Set[m_GraphicsDevice->GetCurrentFrameIndex()], Graphics::g_SceneColor);
		}
		else {
			m_GraphicsDevice->WriteDescriptor(m_InputLayout.bindings[0], m_Set[m_GraphicsDevice->GetCurrentFrameIndex()], Graphics::g_PostEffects);
		}

		m_GraphicsDevice->BindDescriptorSet(m_Set[m_GraphicsDevice->GetCurrentFrameIndex()], frame.commandBuffer, m_Pso.pipelineLayout, 0, 1);
		vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pso.pipeline);
		vkCmdDraw(frame.commandBuffer, 6, 1, 0, 0);

		if (m_UI) {
			m_UI->BeginFrame();
			RenderCoreUI();
			scene.RenderUI();
			m_UI->EndFrame(frame.commandBuffer);
		}

		m_GraphicsDevice->EndRenderPass(frame.commandBuffer);
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
	Graphics::ShutdownRenderingImages();
	Graphics::ShutdownRenderPasses();

	scene.CleanUp();

	PostEffects::Shutdown();

	m_GraphicsDevice->DestroyPipeline(m_Pso);
	m_GraphicsDevice->DestroyDescriptorSetLayout(m_SetLayout);
	m_GraphicsDevice->DestroyShader(m_VertexShader);
	m_GraphicsDevice->DestroyShader(m_FragShader);
}

void Application::Resize(int width, int height) {
	std::cout << "width - " << width << " height - " << height << '\n';

	m_GraphicsDevice->RecreateSwapChain(*m_Window.get());

	Graphics::ResizeDisplayDependentImages(
		m_GraphicsDevice->GetSwapChainExtent().width, 
		m_GraphicsDevice->GetSwapChainExtent().height);

	Graphics::ResizeRenderPasses(
		m_GraphicsDevice->GetSwapChainExtent().width,
		m_GraphicsDevice->GetSwapChainExtent().height);

	m_ResizeApplication = true;
}
