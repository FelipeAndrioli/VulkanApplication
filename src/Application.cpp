#include "Application.h" 

#include "Common.h"
#include "UI.h"
#include "Window.h"
#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "DescriptorSets.h"
#include "ComputePipeline.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include "BufferHelper.h"
#include "Semaphore.h"
#include "Fence.h"
#include "Instance.h"
#include "DebugUtilsMessenger.h"
#include "Material.h"
#include "DepthBuffer.h"

#include "./Input/Input.h"

#include "../Assets/Object.h"
#include "../Assets/Scene.h"

namespace Engine {
	Application::Application(const Settings &settings) : m_Settings(settings) {
		m_Window.reset(new class Window(m_Settings));
		m_Instance.reset(new class Instance(c_ValidationLayers, c_EnableValidationLayers));
		m_DebugMessenger.reset(c_EnableValidationLayers ? new class DebugUtilsMessenger(m_Instance->GetHandle()) : nullptr);

		m_Input.reset(new class InputSystem::Input());

		m_Window->Render = std::bind(&Application::Draw, this);
		m_Window->OnKeyPress = std::bind(&InputSystem::Input::ProcessKey, m_Input.get(), std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4);
		m_Window->OnResize = std::bind(&Application::ProcessResize, this, std::placeholders::_1, std::placeholders::_2);
		m_Window->OnMouseClick = std::bind(&InputSystem::Input::ProcessMouseClick, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		m_Window->OnCursorMove = std::bind(&InputSystem::Input::ProcessCursorMove, m_Input.get(), std::placeholders::_1, std::placeholders::_2);
		m_Window->OnCursorOnScreen = std::bind(&InputSystem::Input::ProcessCursorOnScreen, m_Input.get(), std::placeholders::_1);
	}

	Application::~Application() {
		Shutdown();
	}

	void Application::Init() {
		InitVulkan();

		m_UI.reset(new class UI(
			m_Window->GetHandle(), 
			m_Instance.get(), 
			m_PhysicalDevice.get(), 
			m_LogicalDevice.get(),
			m_SwapChain.get(), 
			MAX_FRAMES_IN_FLIGHT	
		));
	}

	void Application::SetActiveScene(Assets::Scene* scene) {

		if (scene == nullptr) return;

		p_ActiveScene = scene;
		p_ActiveScene->OnCreate();

		m_Window->Update = std::bind(&Application::Update, this, std::placeholders::_1);
	}

	void Application::Run() {
		m_Window->Run();

		m_LogicalDevice->WaitIdle();
	}

	void Application::Update(float t) {

		if (m_Input->Keys[GLFW_KEY_ESCAPE].IsPressed) m_Window->Close();

		if (p_ActiveScene) {
			p_ActiveScene->OnUpdate(t, *m_Input.get());
		}
	}

	void Application::InitVulkan() {
		m_Surface.reset(new class Surface(m_Instance->GetHandle(), *m_Window->GetHandle()));
		m_PhysicalDevice.reset(new class PhysicalDevice(m_Instance->GetHandle(), m_Surface->GetHandle()));
		m_LogicalDevice.reset(new class LogicalDevice(m_Instance.get(), m_PhysicalDevice.get()));
		m_SwapChain.reset(new class SwapChain(m_PhysicalDevice.get(), m_Window.get(), m_LogicalDevice.get(), m_Surface->GetHandle()));
		m_DepthBuffer.reset(new class DepthBuffer(m_PhysicalDevice->GetHandle(), m_LogicalDevice->GetHandle(), *m_SwapChain.get()));

		m_DefaultRenderPass.reset(new class RenderPass(m_SwapChain.get(), m_LogicalDevice->GetHandle(), m_DepthBuffer.get()));
		CreateFramebuffers(m_DefaultRenderPass->GetHandle());

		m_CommandPool.reset(new class CommandPool(m_LogicalDevice->GetHandle(), m_PhysicalDevice->GetQueueFamilyIndices()));	
		m_CommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), 
			m_LogicalDevice->GetHandle()));

		/*
		m_ComputeCommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), 
			m_LogicalDevice->GetHandle()));
		*/
		
		createSyncObjects();
	
		p_ActiveScene->SetupScene(*m_LogicalDevice, *m_PhysicalDevice, *m_CommandPool, *m_SwapChain, *m_DepthBuffer, 
			m_DefaultRenderPass->GetHandle());
		p_ActiveScene->OnResize(m_SwapChain->GetSwapChainExtent().width, m_SwapChain->GetSwapChainExtent().height);
	}

	void Application::Shutdown() {
		m_UI.reset();

		m_SwapChain.reset();

		m_ComputeUniformBuffers.reset();
		//m_ComputePipeline.reset();

		m_DepthBuffer.reset();

		ClearFramebuffers();

		m_DefaultRenderPass.reset();

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

	void Application::createSyncObjects() {
		m_ImageAvailableSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_RenderFinishedSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_ComputeFinishedSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));

		m_InFlightFences.reset(new class Fence(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_ComputeInFlightFences.reset(new class Fence(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
	}

	void Application::recreateSwapChain() {
		m_SwapChain->ReCreate();
		m_DepthBuffer->Resize(m_SwapChain->GetSwapChainExtent().width, m_SwapChain->GetSwapChainExtent().height);

		m_DefaultRenderPass.reset(new class RenderPass(m_SwapChain.get(), m_LogicalDevice->GetHandle(), m_DepthBuffer.get()));

		ClearFramebuffers();
		CreateFramebuffers(m_DefaultRenderPass->GetHandle());

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

	void Application::Draw() {
		VkCommandBuffer* commandBuffer = BeginFrame();

		if (commandBuffer == nullptr) return;

		DrawFrame(*commandBuffer);
		EndFrame(*commandBuffer);
		PresentFrame();
	}

	VkCommandBuffer* Application::BeginFrame() {

		VkResult result;

		vkWaitForFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(m_CurrentFrame), VK_TRUE, UINT64_MAX);
		result = vkAcquireNextImageKHR(m_LogicalDevice->GetHandle(), m_SwapChain->GetHandle(), UINT64_MAX,
			*m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame), VK_NULL_HANDLE, &m_ImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(m_CurrentFrame));

		return &m_CommandBuffers->Begin(m_CurrentFrame);
	}

	void Application::DrawFrame(const VkCommandBuffer& commandBuffer) {
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_DefaultRenderPass->GetHandle();
		renderPassBeginInfo.framebuffer = m_Framebuffers[m_ImageIndex];
		renderPassBeginInfo.renderArea.offset = {0, 0};
		renderPassBeginInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		Material* material = p_ActiveScene->MapMaterials.find("Default")->second;
		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;

		for (Assets::Object* object : p_ActiveScene->Objects) {

			if (object->Material != material) {
				material = p_ActiveScene->MapMaterials.find(object->Material->Layout.ID)->second;

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->GetGraphicsPipeline()->GetHandle());

				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = (float)m_SwapChain->GetSwapChainExtent().width;
				viewport.height = (float)m_SwapChain->GetSwapChainExtent().height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

				VkRect2D scissor = {};
				scissor.offset = { 0, 0 };
				scissor.extent = m_SwapChain->GetSwapChainExtent();

				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

				VkDeviceSize offsets[] = { 0 };

				vkCmdBindVertexBuffers(
					commandBuffer, 
					0, 
					1,
					&material->GetVertexBuffers()->GetBuffer(m_CurrentFrame), 
					offsets
				);
		
				vkCmdBindIndexBuffer(
					commandBuffer, 
					material->GetIndexBuffers()->GetBuffer(), 
					0, 
					VK_INDEX_TYPE_UINT32
				);

				indexOffset = 0;
				vertexOffset = 0;
			}

			vkCmdBindDescriptorSets(
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				material->GetGraphicsPipeline()->GetPipelineLayout().GetHandle(), 
				0, 
				1,
				&object->DescriptorSets->GetDescriptorSet(m_CurrentFrame), 
				0, 
				nullptr
			);

			object->SetObjectUniformBuffer(m_CurrentFrame);

			auto objectVertexCount = object->Meshes->Vertices.size();
			auto objectIndexCount = object->Meshes->Indices.size();

			vkCmdDrawIndexed(
				commandBuffer, 
				static_cast<uint32_t>(objectIndexCount), 
				1, 
				indexOffset, 
				vertexOffset, 
				0
			);
	
			vertexOffset += static_cast<uint32_t>(objectVertexCount);
			indexOffset += static_cast<uint32_t>(objectIndexCount);
		}
		
		vkCmdEndRenderPass(commandBuffer);
	}

	void Application::EndFrame(const VkCommandBuffer& commandBuffer) {
		m_CommandBuffers->End(m_CurrentFrame);

		m_UI->Draw(m_Settings, p_ActiveScene);
		m_UI->RecordCommands(m_CurrentFrame, m_ImageIndex);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		std::array<VkCommandBuffer, 2> cmdBuffers = { m_CommandBuffers->GetCommandBuffer(m_CurrentFrame), m_UI->GetCommandBuffer(m_CurrentFrame) };

		VkSubmitInfo submitInfo{};
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

	void Application::PresentFrame() {
		VkResult result;

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = m_RenderFinishedSemaphores->GetHandle(m_CurrentFrame);
		
		VkSwapchainKHR swapChains[] = { m_SwapChain->GetHandle() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_ImageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(m_LogicalDevice->GetPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
			m_FramebufferResized = false;
			recreateSwapChain();
			//return;
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		// using modulo operator to ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	/* Disabled due to the new architecture, will think in a way to make it work later*/
	void Application::drawRayTraced(VkCommandBuffer& p_CommandBuffer, uint32_t imageIndex) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		vkWaitForFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame), VK_TRUE, UINT64_MAX);
		updateComputeUniformBuffer(m_CurrentFrame);
		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame));

		auto computeCommandBuffer = m_ComputeCommandBuffers->Begin(m_CurrentFrame);
		//recordComputeCommandBuffer(computeCommandBuffer);
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
		//renderPassInfo.renderPass = m_TempRayTracerPipeline->GetRenderPass().GetHandle();
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

	void Application::ProcessResize(int width, int height) {
		m_FramebufferResized = true;
		std::cout << "width - " << width << " height - " << height << '\n';

		p_ActiveScene->OnResize(width, height);
	}

	void Application::CreateFramebuffers(const VkRenderPass& renderPass) {
		m_Framebuffers.resize(m_SwapChain->GetSwapChainImageViews().size());

		for (size_t i = 0; i < m_SwapChain->GetSwapChainImageViews().size(); i++) {
			std::array<VkImageView, 2> attachments = { 
				m_SwapChain->GetSwapChainImageViews()[i], 
				m_DepthBuffer->GetDepthBufferImageView()[0] 
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_SwapChain->GetSwapChainExtent().width;
			framebufferInfo.height = m_SwapChain->GetSwapChainExtent().height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_LogicalDevice->GetHandle(), &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void Application::ClearFramebuffers() {
		for (VkFramebuffer framebuffer : m_Framebuffers) {
			vkDestroyFramebuffer(m_LogicalDevice->GetHandle(), framebuffer, nullptr);
		}

		m_Framebuffers.clear();
	}

	/* TODO: not ready AON
	void Application::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline->GetHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline->GetPipelineLayout().GetHandle(), 
			0, 1,&m_ComputePipeline->GetDescriptorSets().GetDescriptorSet(m_CurrentFrame), 
			0, nullptr);
		vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);
	}
	*/
}
