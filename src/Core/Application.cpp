#include "Application.h"

#include "BufferManager.h"

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

	/*
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
	//desc.flags = Graphics::RenderPass::tDepthAttachment | Graphics::RenderPass::tColorResolveAttachment;

	m_GraphicsDevice->CreateRenderPass(desc, scene.renderPass);
	*/

	CreateRenderPass(scene);

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

	scene.RenderScene(m_GraphicsDevice->GetCurrentFrameIndex(), frame.commandBuffer);

	m_GraphicsDevice->BeginRenderPass(scene.renderPass, frame.commandBuffer);

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
	Graphics::ShutdownRenderingImages();

	m_GraphicsDevice->DestroyRenderPass(scene.renderPass);
	scene.CleanUp();
}

void Application::Resize(int width, int height) {
	std::cout << "width - " << width << " height - " << height << '\n';

	m_GraphicsDevice->RecreateSwapChain(*m_Window.get());

	Graphics::ResizeDisplayDependentImages(
		m_GraphicsDevice->GetSwapChainExtent().width, 
		m_GraphicsDevice->GetSwapChainExtent().height);

	m_ResizeApplication = true;
}

void Application::CreateRenderPass(IScene& scene) {
	Graphics::RenderPassDesc desc = {};
	desc.scissor.offset = { 0, 0 };
	desc.scissor.extent = m_GraphicsDevice->GetSwapChain().swapChainExtent;
	desc.extent = m_GraphicsDevice->GetSwapChain().swapChainExtent;
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

	scene.renderPass.description = desc;

	std::vector<VkAttachmentDescription> attachments = {};
	std::vector<VkSubpassDependency> dependencies(3);
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_GraphicsDevice->GetSwapChain().swapChainImageFormat;
	//colorAttachment.samples = m_GraphicsDevice->m_MsaaSamples;
	colorAttachment.samples = Graphics::g_FinalImage.Description.MsaaSamples;
//	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	attachments.emplace_back(colorAttachment);

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = attachments.size() - 1;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = m_GraphicsDevice->GetDepthFormat();
	//depthAttachment.samples = m_GraphicsDevice->m_MsaaSamples;
	depthAttachment.samples = Graphics::g_FinalDepth.Description.MsaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments.emplace_back(depthAttachment);

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = attachments.size() - 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = m_GraphicsDevice->GetSwapChain().swapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments.emplace_back(colorAttachmentResolve);

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = attachments.size() - 1;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency colorDependency = {};
	colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	colorDependency.dstSubpass = 0;
	colorDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	colorDependency.srcAccessMask = VK_ACCESS_NONE_KHR;
	colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	colorDependency.dependencyFlags = 0;

	dependencies[0] = colorDependency;

	VkSubpassDependency depthDependency = {};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	depthDependency.dependencyFlags = 0;

	dependencies[1] = depthDependency;

	VkSubpassDependency resolveDependency = {};
	resolveDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	resolveDependency.dstSubpass = 0;
	resolveDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	resolveDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	resolveDependency.srcAccessMask = 0;
	resolveDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	resolveDependency.dependencyFlags = 0;

	dependencies[2] = resolveDependency;

	std::vector<VkSubpassDescription> subpasses = { subpass };

	m_GraphicsDevice->CreateRenderPass(scene.renderPass.handle, attachments, subpasses, dependencies);

	scene.renderPass.framebuffers.resize(m_GraphicsDevice->GetSwapChain().swapChainImageViews.size());

	for (int i = 0; i < m_GraphicsDevice->GetSwapChain().swapChainImageViews.size(); i++) {
		std::vector<VkImageView> framebufferAttachments = {};
		framebufferAttachments.emplace_back(Graphics::g_FinalImage.ImageView);
		framebufferAttachments.emplace_back(Graphics::g_FinalDepth.ImageView);
		framebufferAttachments.emplace_back(m_GraphicsDevice->GetSwapChain().swapChainImageViews[i]);

		m_GraphicsDevice->CreateFramebuffer(
			scene.renderPass.handle, 
			framebufferAttachments, 
			scene.renderPass.description.extent, 
			scene.renderPass.framebuffers[i]);
	}
}
