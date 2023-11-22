#include "Application.h" 

static bool g_FramebufferResized = false;

static int g_MinImageCount = 2;

namespace Engine {
	Application::Application(const WindowSettings &windowSettings, const UserSettings &userSettings) 
		: m_WindowSettings(windowSettings), m_UserSettings(userSettings) {
		m_Window.reset(new class Window(&m_WindowSettings));
		m_Instance.reset(new class Instance(c_ValidationLayers, c_EnableValidationLayers));
		m_DebugMessenger.reset(c_EnableValidationLayers ? new class DebugUtilsMessenger(m_Instance->GetHandle()) : nullptr);
	}

	Application::~Application() {
		Shutdown();
	}

	/* Disabled due to the new architecture, will think in a way to make it work later*/
	void Application::drawRayTraced(VkCommandBuffer& p_CommandBuffer, uint32_t imageIndex) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		vkWaitForFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame), VK_TRUE, UINT64_MAX);
		updateComputeUniformBuffer(m_CurrentFrame);
		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame));

		auto computeCommandBuffer = m_ComputeCommandBuffers->Begin(m_CurrentFrame);
		recordComputeCommandBuffer(computeCommandBuffer);
		m_ComputeCommandBuffers->End(m_CurrentFrame);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_ComputeCommandBuffers->GetCommandBuffer(m_CurrentFrame);
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = m_ComputeFinishedSemaphores->GetHandle(m_CurrentFrame);

		if (vkQueueSubmit(m_LogicalDevice->GetComputeQueue(), 1, &submitInfo, *m_ComputeInFlightFences->GetHandle(m_CurrentFrame)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit compute command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_TempRayTracerPipeline->GetRenderPass().GetHandle();
		//renderPassInfo.framebuffer = m_SwapChain->GetSwapChainFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(p_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TempRayTracerPipeline->GetHandle());

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChain->GetSwapChainExtent().width;
		viewport.height = (float)m_SwapChain->GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(p_CommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChain->GetSwapChainExtent();

		vkCmdSetScissor(p_CommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1, &m_ShaderStorageBuffers->GetBuffer(m_CurrentFrame), offsets);
		vkCmdDraw(p_CommandBuffer, PARTICLE_COUNT, 1, 0, 0);
		vkCmdEndRenderPass(p_CommandBuffer);
	}

	void Application::drawRasterized(VkCommandBuffer& p_CommandBuffer, uint32_t imageIndex) {

		vkCmdBeginRenderPass(p_CommandBuffer, p_ActiveScene->RenderPassBeginInfo[imageIndex], 
			VK_SUBPASS_CONTENTS_INLINE);

		ResourceSet* resourceSet = nullptr;
		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;

		for (Assets::Model* model : p_ActiveScene->Models) {

			if (resourceSet == nullptr || model->ResourceSetIndex != resourceSet->ResourceSetIndex) {
				resourceSet = p_ActiveScene->ResourceSets[model->ResourceSetIndex];

				vkCmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, resourceSet->GetGraphicsPipeline()->GetHandle());

				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = (float)m_SwapChain->GetSwapChainExtent().width;
				viewport.height = (float)m_SwapChain->GetSwapChainExtent().height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				vkCmdSetViewport(p_CommandBuffer, 0, 1, &viewport);

				VkRect2D scissor = {};
				scissor.offset = { 0, 0 };
				scissor.extent = m_SwapChain->GetSwapChainExtent();

				vkCmdSetScissor(p_CommandBuffer, 0, 1, &scissor);

				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1,
					&resourceSet->GetVertexBuffers()->GetBuffer(m_CurrentFrame), offsets);
				vkCmdBindIndexBuffer(p_CommandBuffer, resourceSet->GetIndexBuffers()->GetBuffer(), 0,
					VK_INDEX_TYPE_UINT16);

				indexOffset = 0;
				vertexOffset = 0;
			}

			vkCmdBindDescriptorSets(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
				resourceSet->GetGraphicsPipeline()->GetPipelineLayout().GetHandle(), 0, 1,
				&model->m_DescriptorSets->GetDescriptorSet(m_CurrentFrame), 0, 
				nullptr);

			model->SetModelUniformBuffer(m_CurrentFrame);

			auto modelVertexCount = model->GetSizeVertices();
			auto modelIndexCount = model->GetSizeIndices();

			vkCmdDrawIndexed(p_CommandBuffer, modelIndexCount, 1, indexOffset, vertexOffset, 0);
		
			vertexOffset += modelVertexCount;
			indexOffset += modelIndexCount;
		}
		
		vkCmdEndRenderPass(p_CommandBuffer);
	}

	void Application::handleDraw(uint32_t imageIndex) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(m_CurrentFrame));

		auto commandBuffer = m_CommandBuffers->Begin(m_CurrentFrame);

		drawRasterized(commandBuffer, imageIndex);
		
		m_CommandBuffers->End(m_CurrentFrame);

		m_UI->RecordCommands(m_CurrentFrame, imageIndex);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		std::array<VkCommandBuffer, 2> cmdBuffers = { m_CommandBuffers->GetCommandBuffer(m_CurrentFrame), m_UI->GetCommandBuffer(m_CurrentFrame) };

		submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame);

		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submitInfo.pCommandBuffers = cmdBuffers.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = m_RenderFinishedSemaphores->GetHandle(m_CurrentFrame);
	
		if (vkQueueSubmit(m_LogicalDevice->GetGraphicsQueue(), 1, &submitInfo, *m_InFlightFences->GetHandle(m_CurrentFrame)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}
	}
	
	void Application::drawFrame() {

		m_UI->Draw(m_UserSettings, m_WindowSettings, p_ActiveScene);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		uint32_t imageIndex;
		VkResult result;

		vkWaitForFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(m_CurrentFrame), VK_TRUE, UINT64_MAX);
		result = vkAcquireNextImageKHR(m_LogicalDevice->GetHandle(), m_SwapChain->GetHandle(), UINT64_MAX,
			*m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame), VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		handleDraw(imageIndex);

		// Present submit
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = m_RenderFinishedSemaphores->GetHandle(m_CurrentFrame);
		
		VkSwapchainKHR swapChains[] = { m_SwapChain->GetHandle() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(m_LogicalDevice->GetPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || g_FramebufferResized) {
			g_FramebufferResized = false;
			recreateSwapChain();
			//return;
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		// using modulo operator to ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Application::SetActiveScene(Assets::Scene* scene) {
		p_ActiveScene = scene;
		p_ActiveScene->OnCreate();

		m_Window->Update = std::bind(&Application::Update, this, std::placeholders::_1);
	}

	void Application::Run() {

		m_Window->DrawFrame = std::bind(&Application::drawFrame, this);
		m_Window->OnKeyPress = std::bind(&Application::processKey, this, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4);
		m_Window->OnResize = std::bind(&Application::processResize, this, std::placeholders::_1, std::placeholders::_2);
		m_Window->Run();

		m_LogicalDevice->WaitIdle();
	}

	void Application::Update(float t) {
		if (p_ActiveScene) {
			p_ActiveScene->OnUpdate(t);
		}
	}

	void Application::Init() {
		InitVulkan();

		m_UI.reset(new class UI(m_Window->GetHandle(), m_Instance.get(), m_PhysicalDevice.get(), m_LogicalDevice.get(),
			m_SwapChain.get(), g_MinImageCount));
	}

	void Application::processKey(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			std::cout << "Closing application" << '\n';
			m_Window->Close();
		}
	}

	void Application::processResize(int width, int height) {
		g_FramebufferResized = true;
		std::cout << "width - " << width << " height - " << height << '\n';
	}

	void Application::InitVulkan() {
		m_Surface.reset(new class Surface(m_Instance->GetHandle(), *m_Window->GetHandle()));
		m_PhysicalDevice.reset(new class PhysicalDevice(m_Instance->GetHandle(), m_Surface->GetHandle()));
		m_LogicalDevice.reset(new class LogicalDevice(m_Instance.get(), m_PhysicalDevice.get()));
		m_SwapChain.reset(new class SwapChain(m_PhysicalDevice.get(), m_Window.get(), m_LogicalDevice.get(), m_Surface->GetHandle()));
		m_CommandPool.reset(new class CommandPool(m_LogicalDevice->GetHandle(), m_PhysicalDevice->GetQueueFamilyIndices()));	

		p_ActiveScene->SetupScene(m_LogicalDevice.get(), m_PhysicalDevice.get(), m_CommandPool.get(), m_SwapChain.get());

		m_CommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), 
			m_LogicalDevice->GetHandle()));
		m_ComputeCommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), 
			m_LogicalDevice->GetHandle()));

		createSyncObjects();
	}

	void Application::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline->GetHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline->GetPipelineLayout().GetHandle(), 
			0, 1,&m_ComputePipeline->GetDescriptorSets().GetDescriptorSet(m_CurrentFrame), 
			0, nullptr);
		vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);
	}

	void Application::createSyncObjects() {
		// TODO: The Semaphore and Fence classes are pretty similar, maybe we can turn them into a template instead of two different classes

		m_ImageAvailableSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_RenderFinishedSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_ComputeFinishedSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));

		m_InFlightFences.reset(new class Fence(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_ComputeInFlightFences.reset(new class Fence(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));

	}

	void Application::recreateSwapChain() {
		m_SwapChain->ReCreate();
		p_ActiveScene->Resize(m_SwapChain.get());
		m_CommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(),
			m_LogicalDevice->GetHandle()));
		m_UI->Resize(m_SwapChain.get());
	}

	// temporary
	void Application::updateComputeUniformBuffer(uint32_t currentImage) {
		ComputeUniformBufferObject cubo{};
		cubo.deltaTime = m_Window->GetLastFrameTime() * 0.1f;

		//memcpy(m_ComputeUniformBuffers->GetBufferMemoryMapped(currentImage), &cubo, sizeof(cubo));
	}

	void Application::Shutdown() {
		m_UI.reset();

		m_SwapChain.reset();

		m_ComputeUniformBuffers.reset();
		m_ComputePipeline.reset();

		m_CommandBuffers.reset();
		m_ComputeCommandBuffers.reset();

		m_GraphicsPipeline.reset();
		m_TempRayTracerPipeline.reset();

		m_IndexBuffer.reset();
		m_ShaderStorageBuffers.reset();
		m_VertexBuffers.reset();

		m_DebugMessenger.reset();
		m_InFlightFences.reset();
		m_ComputeInFlightFences.reset();

		m_ImageAvailableSemaphores.reset();
		m_RenderFinishedSemaphores.reset();
		m_ComputeFinishedSemaphores.reset();

		m_CommandPool.reset();

		m_LogicalDevice.reset();
		m_Surface.reset();
		m_Instance.reset();
		
		glfwTerminate();
	}
}
