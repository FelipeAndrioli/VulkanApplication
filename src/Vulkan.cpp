#include "Vulkan.h"
#include "Surface.h"
#include "Instance.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "RenderPass.h"
#include "DebugUtilsMessenger.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Semaphore.h"
#include "Fence.h"
#include "Window.h"
#include "UI.h"

namespace Engine {
	VulkanEngine::VulkanEngine(Window& window) {
		m_Instance = std::make_unique<class Instance>(c_ValidationLayers, c_EnableValidationLayers);

		if (c_EnableValidationLayers)
			m_DebugMessenger = std::make_unique<class DebugUtilsMessenger>(m_Instance->GetHandle());

		m_Surface = std::make_unique<class Surface>(m_Instance->GetHandle(), *window.GetHandle());
		m_PhysicalDevice = std::make_unique<class PhysicalDevice>(m_Instance->GetHandle(), m_Surface->GetHandle());
		m_LogicalDevice = std::make_unique<class LogicalDevice>(m_Instance.get(), m_PhysicalDevice.get());
		m_SwapChain = std::make_unique<class SwapChain>(m_PhysicalDevice.get(), &window, m_LogicalDevice.get(), m_Surface->GetHandle());
		m_DepthBuffer = std::make_unique<class DepthBuffer>(m_PhysicalDevice->GetHandle(), m_LogicalDevice->GetHandle(), *m_SwapChain.get());
		m_DefaultRenderPass = std::make_unique<class RenderPass>(m_SwapChain.get(), m_LogicalDevice->GetHandle(), m_DepthBuffer.get());
	
		CreateSyncObjects();
		CreateFramebuffers(m_DefaultRenderPass->GetHandle());
		
		m_CommandPool = std::make_unique<class CommandPool>(m_LogicalDevice->GetHandle(), m_PhysicalDevice->GetQueueFamilyIndices());	
		m_CommandBuffers = std::make_unique<class CommandBuffer>(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), m_LogicalDevice->GetHandle());

		m_UI = std::make_unique<class UI>(
			window.GetHandle(),
			m_Instance.get(),
			m_PhysicalDevice.get(),
			m_LogicalDevice.get(),
			m_SwapChain.get(),
			MAX_FRAMES_IN_FLIGHT	
		);
	}

	VulkanEngine::~VulkanEngine() {
		m_SwapChain.reset();
		m_DepthBuffer.reset();

		ClearFramebuffers();

		m_UI.reset();
		m_DefaultRenderPass.reset();
		m_CommandBuffers.reset();
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
	}
		
	void VulkanEngine::CreateFramebuffers(const VkRenderPass& renderPass) {
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

	void VulkanEngine::ClearFramebuffers() {
		for (VkFramebuffer framebuffer : m_Framebuffers) {
			vkDestroyFramebuffer(m_LogicalDevice->GetHandle(), framebuffer, nullptr);
		}

		m_Framebuffers.clear();
	}

	void VulkanEngine::CreateSyncObjects() {
		m_ImageAvailableSemaphores = std::make_unique<class Semaphore>(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores = std::make_unique<class Semaphore>(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT);
		m_ComputeFinishedSemaphores = std::make_unique<class Semaphore>(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT);

		m_InFlightFences = std::make_unique<class Fence>(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT);
		m_ComputeInFlightFences = std::make_unique<class Fence>(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT);
	}

	void VulkanEngine::Resize() {
		m_SwapChain->ReCreate();
		m_DepthBuffer->Resize(m_SwapChain->GetSwapChainExtent().width, m_SwapChain->GetSwapChainExtent().height);

		m_DefaultRenderPass.reset();
		m_DefaultRenderPass = std::make_unique<class RenderPass>(m_SwapChain.get(), m_LogicalDevice->GetHandle(), m_DepthBuffer.get());

		ClearFramebuffers();
		CreateFramebuffers(m_DefaultRenderPass->GetHandle());

		m_CommandBuffers.reset();
		m_CommandBuffers = std::make_unique<class CommandBuffer>(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), m_LogicalDevice->GetHandle());
	
		m_UI->Resize(m_SwapChain.get());
	}

	VkResult VulkanEngine::PrepareNextImage(uint32_t& currentFrame, uint32_t& imageIndex) {
		VkResult result = vkAcquireNextImageKHR(
			m_LogicalDevice->GetHandle(), 
			m_SwapChain->GetHandle(), 
			UINT64_MAX,
			*m_ImageAvailableSemaphores->GetHandle(currentFrame), 
			VK_NULL_HANDLE, 
			&imageIndex
		);

		return result;
	}

	VkCommandBuffer* VulkanEngine::BeginFrame(uint32_t& currentFrame, uint32_t& imageIndex) {

		VkResult result;

		vkWaitForFences(
			m_LogicalDevice->GetHandle(),
			1, 
			m_InFlightFences->GetHandle(currentFrame),
			VK_TRUE, 
			UINT64_MAX
		);
		
		result = PrepareNextImage(currentFrame, imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return nullptr;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(currentFrame));

		return &m_CommandBuffers->Begin(currentFrame);
	}

	void VulkanEngine::EndFrame(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, uint32_t frameIndex) {
		//m_CommandBuffers->End(m_CurrentFrame);
		m_CommandBuffers->End(commandBuffer);

		//m_UI->Draw(m_Settings, p_ActiveScene);
		//m_UI->RecordCommands(m_CurrentFrame, m_ImageIndex);
		m_UI->RecordCommandBuffer(currentFrame, frameIndex);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		std::array<VkCommandBuffer, 2> cmdBuffers = { commandBuffer, m_UI->GetCommandBuffer(currentFrame) };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = m_ImageAvailableSemaphores->GetHandle(currentFrame);

		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submitInfo.pCommandBuffers = cmdBuffers.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = m_RenderFinishedSemaphores->GetHandle(currentFrame);

		VkResult result = vkQueueSubmit(
			m_LogicalDevice->GetGraphicsQueue(),
			1,
			&submitInfo,
			*m_InFlightFences->GetHandle(currentFrame)
		);
		
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}
	}

	void VulkanEngine::PresentFrame(uint32_t& currentFrame, uint32_t& imageIndex) {
		VkResult result;

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = m_RenderFinishedSemaphores->GetHandle(currentFrame);
		
		VkSwapchainKHR swapChains[] = { m_SwapChain->GetHandle() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(m_LogicalDevice->GetPresentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
			return;
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		// using modulo operator to ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanEngine::BeginUIFrame(Settings& settings) {
		m_UI->BeginFrame(settings);
	}

	void VulkanEngine::EndUIFrame() {
		m_UI->EndFrame();
	}
}
