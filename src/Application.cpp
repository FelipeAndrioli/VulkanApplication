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
#include "DepthBuffer.h"

#include "./Input/Input.h"

#include "./Utils/ModelLoader.h"
#include "./Utils/TextureLoader.h"

#include "../Assets/Object.h"
#include "../Assets/Scene.h"
#include "../Assets/Pipeline.h"
#include "../Assets/Camera.h"
#include "../Assets/Material.h"

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
		// Init Vulkan Essentials
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

		createSyncObjects();

		// Load models/materials/textures
		for (Assets::Object* renderableObject : p_ActiveScene->RenderableObjects) {
			Utils::ModelLoader::LoadModelAndMaterials(
				*renderableObject,
				m_Materials,
				m_LoadedTextures,
				*m_LogicalDevice.get(),
				*m_PhysicalDevice.get(),
				*m_CommandPool.get()
			);
		}
		
		p_ActiveScene->Setup();
		p_ActiveScene->OnResize(m_SwapChain->GetSwapChainExtent().width, m_SwapChain->GetSwapChainExtent().height);

		InitializeBuffers();
		InitializeDescriptors();
		
		CreatePipelineLayouts();
		CreateGraphicsPipelines();
	}

	void Application::Shutdown() {

		m_ObjectGPUDataDescriptorSetLayout.reset();
		m_GlobalDescriptorSetLayout.reset();

		m_GlobalDescriptorSets.reset();
		m_GPUDataBuffer.reset();
		m_SceneGeometryBuffer.reset();

		m_MainGraphicsPipelineLayout.reset();
		m_TexturedPipeline.reset();

		m_Materials.clear();
		m_LoadedTextures.clear();
		m_DescriptorPool.reset();
		m_UI.reset();
		m_SwapChain.reset();
		m_DepthBuffer.reset();

		ClearFramebuffers();

		m_DefaultRenderPass.reset();
		m_CommandBuffers.reset();
		m_ShaderStorageBuffers.reset();
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

		m_CommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), m_LogicalDevice->GetHandle()));
		m_UI->Resize(m_SwapChain.get());
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

		VkDeviceSize offsets[] = { p_ActiveScene->VertexOffset };

		vkCmdBindVertexBuffers(
			commandBuffer,
			0,
			1,
			&m_SceneGeometryBuffer->GetBuffer(m_CurrentFrame),
			offsets
		);
		
		vkCmdBindIndexBuffer(
			commandBuffer,
			m_SceneGeometryBuffer->GetBuffer(m_CurrentFrame),
			0,
			VK_INDEX_TYPE_UINT32
		);

		SceneGPUData sceneGPUData = SceneGPUData();
		sceneGPUData.view = p_ActiveScene->MainCamera->ViewMatrix;
		sceneGPUData.proj = p_ActiveScene->MainCamera->ProjectionMatrix;

		VkDeviceSize sceneBufferOffset = m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize + m_GPUDataBuffer->Chunks[MATERIAL_BUFFER_INDEX].ChunkSize;
		m_GPUDataBuffer->Update(m_CurrentFrame, sceneBufferOffset, &sceneGPUData, sizeof(SceneGPUData));

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TexturedPipeline->GetHandle());

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

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_MainGraphicsPipelineLayout->GetHandle(),
			1,
			1,
			&m_GlobalDescriptorSets->GetDescriptorSet(m_CurrentFrame),
			0,
			nullptr
		);

		for (size_t i = 0; i < p_ActiveScene->RenderableObjects.size(); i++) {
			Assets::Object* object = p_ActiveScene->RenderableObjects[i];

			vkCmdBindDescriptorSets(
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				m_MainGraphicsPipelineLayout->GetHandle(),
				0, 
				1,
				&object->DescriptorSets->GetDescriptorSet(m_CurrentFrame),
				0, 
				nullptr
			);

			ObjectGPUData objectGPUData = ObjectGPUData();
			objectGPUData.model = object->GetModelMatrix();

			VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
			m_GPUDataBuffer->Update(m_CurrentFrame, objectBufferOffset, &objectGPUData, sizeof(ObjectGPUData));

			Assets::Material* material = nullptr;

			for (const Assets::Mesh* mesh : object->Meshes) {
				vkCmdPushConstants(
					commandBuffer,
					m_MainGraphicsPipelineLayout->GetHandle(),
					VK_SHADER_STAGE_FRAGMENT_BIT,
					0,
					sizeof(int),
					&mesh->MaterialIndex
				);

				vkCmdDrawIndexed(
					commandBuffer,
					static_cast<uint32_t>(mesh->Indices.size()),
					1,
					static_cast<uint32_t>(mesh->IndexOffset),
					static_cast<int32_t>(mesh->VertexOffset),
					0
				);
			}
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

		VkResult result = vkQueueSubmit(m_LogicalDevice->GetGraphicsQueue(), 1, &submitInfo, *m_InFlightFences->GetHandle(m_CurrentFrame));
		
		if (result != VK_SUCCESS) {
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
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		// using modulo operator to ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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

	void Application::InitializeBuffers() {
		// GPU Data Buffer Begin
		VkDeviceSize objectBufferSize = sizeof(ObjectGPUData) * p_ActiveScene->RenderableObjects.size();
		VkDeviceSize materialsBufferSize = sizeof(Assets::MeshMaterialData) * m_Materials.size();
		VkDeviceSize sceneBufferSize = sizeof(SceneGPUData);
		
		m_GPUDataBuffer.reset(new class Engine::Buffer(
			Engine::MAX_FRAMES_IN_FLIGHT,
			*m_LogicalDevice.get(),
			*m_PhysicalDevice.get(),
			objectBufferSize + materialsBufferSize + sceneBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		));
		m_GPUDataBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		m_GPUDataBuffer->NewChunk({ sizeof(ObjectGPUData), objectBufferSize});
		m_GPUDataBuffer->NewChunk({ sizeof(Assets::MeshMaterialData), materialsBufferSize});
		m_GPUDataBuffer->NewChunk({ sizeof(SceneGPUData), sceneBufferSize});

		std::vector<Assets::MeshMaterialData> meshMaterialData;

		for (const auto& material : m_Materials) {
			meshMaterialData.push_back(material.MaterialData);
		}

		Engine::BufferHelper::AppendData(
			*m_LogicalDevice.get(),
			*m_PhysicalDevice.get(),
			*m_CommandPool.get(),
			meshMaterialData,
			*m_GPUDataBuffer.get(),
			0,
			m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize
		);
		// GPU Data Buffer End

		// Scene Geometry Buffer Begin
		VkDeviceSize bufferSize = sizeof(uint32_t) * p_ActiveScene->Indices.size() 
			+ sizeof(Assets::Vertex) * p_ActiveScene->Vertices.size();
		
		m_SceneGeometryBuffer.reset(new class Engine::Buffer(
			Engine::MAX_FRAMES_IN_FLIGHT,
			*m_LogicalDevice.get(),
			*m_PhysicalDevice.get(),
			bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		));
		m_SceneGeometryBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_SceneGeometryBuffer->NewChunk({ sizeof(uint32_t), sizeof(uint32_t) * p_ActiveScene->Indices.size() });
		m_SceneGeometryBuffer->NewChunk({ sizeof(Assets::Vertex), sizeof(Assets::Vertex) * p_ActiveScene->Vertices.size() });

		Engine::BufferHelper::AppendData(
			*m_LogicalDevice.get(),
			*m_PhysicalDevice.get(),
			*m_CommandPool.get(),
			p_ActiveScene->Indices,
			*m_SceneGeometryBuffer.get(),
			0,
			0
		);

		Engine::BufferHelper::AppendData(
			*m_LogicalDevice.get(),
			*m_PhysicalDevice.get(),
			*m_CommandPool.get(),
			p_ActiveScene->Vertices,
			*m_SceneGeometryBuffer.get(),
			0,
			m_SceneGeometryBuffer->Chunks[INDEX_BUFFER_INDEX].ChunkSize
		);
		// Scene Geometry Buffer End
	}

	void Application::InitializeDescriptors() {
		std::vector<PoolDescriptorBinding> poolDescriptorBindings = {};

		size_t maxDescriptorSets = 50;
		uint32_t buffers = 10;

		for (size_t i = 0; i < maxDescriptorSets; i++) {
			poolDescriptorBindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffers });
			poolDescriptorBindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffers });
			poolDescriptorBindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, buffers });

			// We need to double the number of VK_DESCRIPTOR_TYPE_STORAGE_BUFFER types requested from the pool
			// because our sets reference the SSBOs of the last and current frame (for now).

			poolDescriptorBindings.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, buffers });
		}

		m_DescriptorPool.reset(new class DescriptorPool(
			m_LogicalDevice->GetHandle(), 
			poolDescriptorBindings, 
			static_cast<uint32_t>(maxDescriptorSets * MAX_FRAMES_IN_FLIGHT)));

		DescriptorSetLayoutBuild descriptorLayoutBuild = {};
		m_ObjectGPUDataDescriptorSetLayout = descriptorLayoutBuild.NewBinding(0)
			.SetDescriptorCount(1)
			.SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			.SetStage(VK_SHADER_STAGE_VERTEX_BIT)
			.SetResource(*m_GPUDataBuffer.get())
			.SetBufferSize(m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize)
			.SetBufferOffset(0)
			.Add()
			.Build(m_LogicalDevice->GetHandle());

		m_GlobalDescriptorSetLayout = descriptorLayoutBuild.NewBinding(0)
			.SetDescriptorCount(1)
			.SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			.SetStage(VK_SHADER_STAGE_VERTEX_BIT)
			.SetResource(*m_GPUDataBuffer.get())
			.SetBufferSize(m_GPUDataBuffer->Chunks[SCENE_BUFFER_INDEX].ChunkSize)
			.SetBufferOffset(m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize + m_GPUDataBuffer->Chunks[MATERIAL_BUFFER_INDEX].ChunkSize)
			.Add()
			.NewBinding(1)
			.SetDescriptorCount(1)
			.SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			.SetStage(VK_SHADER_STAGE_FRAGMENT_BIT)
			.SetResource(*m_GPUDataBuffer.get())
			.SetBufferSize(m_GPUDataBuffer->Chunks[MATERIAL_BUFFER_INDEX].ChunkSize)
			.SetBufferOffset(m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize)
			.Add()
			.NewBinding(2)
			.SetDescriptorCount(static_cast<uint32_t>(m_LoadedTextures.size()))
			.SetType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.SetStage(VK_SHADER_STAGE_FRAGMENT_BIT)
			.SetResource(m_LoadedTextures)
			.SetBufferSize(0)
			.SetBufferOffset(0)
			.Add()
			.Build(m_LogicalDevice->GetHandle());

		// Renderable Objects Descriptor Sets Begin
		for (size_t i = 0; i < p_ActiveScene->RenderableObjects.size(); i++) {
			Assets::Object* renderableObject = p_ActiveScene->RenderableObjects[i];
			VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;

			m_ObjectGPUDataDescriptorSetLayout->UpdateOffset(0, objectBufferOffset);

			renderableObject->DescriptorSets.reset(new class Engine::DescriptorSets(
				m_LogicalDevice->GetHandle(),
				m_DescriptorPool->GetHandle(),
				*m_ObjectGPUDataDescriptorSetLayout.get()
			));
		}
		// Renderable Objects Descriptor Sets End 

		// Global Descriptor Sets Begin
		m_GlobalDescriptorSets.reset(new class DescriptorSets(
			m_LogicalDevice->GetHandle(),
			m_DescriptorPool->GetHandle(),
			*m_GlobalDescriptorSetLayout.get()
		));
		// Global Descriptor Sets End
	}

	void Application::CreatePipelineLayouts() {

		m_MainGraphicsPipelineLayout.reset(new class PipelineLayout(
			m_LogicalDevice->GetHandle(), 
			{ 
				m_ObjectGPUDataDescriptorSetLayout.get(), 
				m_GlobalDescriptorSetLayout.get()
			}
		));
	}

	void Application::CreateGraphicsPipelines() {

		/*
		Assets::VertexShader wireframeVertexShader = Assets::VertexShader("Default Vertex Shader", "./Assets/Shaders/textured_vert.spv");
		Assets::FragmentShader wireframeFragShader = Assets::FragmentShader("Wireframe Fragment Shader", "./Assets/Shaders/textured_frag.spv");
		wireframeFragShader.PolygonMode = Assets::FragmentShader::Polygon::LINE;
		*/

		Assets::VertexShader texturedVertexShader = Assets::VertexShader("Textured Vertex Shader", "C:/Users/Felipe/Documents/current_projects/VulkanApplication/Assets/Shaders/textured_vert.spv");
		Assets::FragmentShader texturedFragmentShader = Assets::FragmentShader("Textured Fragment Shader", "C:/Users/Felipe/Documents/current_projects/VulkanApplication/Assets/Shaders/textured_frag.spv");

		m_TexturedPipeline.reset(new class GraphicsPipeline(
			texturedVertexShader,
			texturedFragmentShader,
			*m_LogicalDevice,
			*m_SwapChain,
			*m_DepthBuffer,
			m_DefaultRenderPass->GetHandle(),
			*m_MainGraphicsPipelineLayout
		));
	}
}
