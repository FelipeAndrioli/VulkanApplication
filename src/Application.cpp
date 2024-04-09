#include "Application.h" 

#include "Vulkan.h"
#include "Common.h"
#include "UI.h"

#include "Window.h"
#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "PipelineLayout.h"
#include "Pipeline.h"
#include "DescriptorSetLayout.h"
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
	}

	void Application::SetActiveScene(Assets::Scene* scene) {

		if (scene == nullptr) return;

		p_ActiveScene = scene;
		p_ActiveScene->OnCreate();

		m_Window->Update = std::bind(&Application::Update, this, std::placeholders::_1);
	}

	void Application::Run() {
		m_Window->Run();
		m_VulkanEngine->GetLogicalDevice().WaitIdle();
	}

	void Application::Update(float t) {

		if (m_Input->Keys[GLFW_KEY_ESCAPE].IsPressed) m_Window->Close();

		if (p_ActiveScene) {
			p_ActiveScene->OnUpdate(t, *m_Input.get());
		}
	}

	void Application::InitVulkan() {
		m_VulkanEngine = std::make_unique<class VulkanEngine>(*m_Window.get(), m_Settings);

		// Load models/materials/textures
		for (Assets::Object* renderableObject : p_ActiveScene->RenderableObjects) {
			Utils::ModelLoader::LoadModelAndMaterials(
				*renderableObject,
				m_Materials,
				m_LoadedTextures,
				*m_VulkanEngine.get()
			);
		}
		
		p_ActiveScene->Setup();
		p_ActiveScene->OnResize(
			m_VulkanEngine->GetSwapChain().GetSwapChainExtent().width,
			m_VulkanEngine->GetSwapChain().GetSwapChainExtent().height
		);

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

		m_MainPipelineLayout.reset();
		m_TexturedPipeline.reset();
		m_WireframePipeline.reset();
		m_ColoredPipeline.reset();

		m_Materials.clear();
		m_LoadedTextures.clear();
		m_DescriptorPool.reset();
		m_VulkanEngine.reset();
		
		glfwTerminate();
	}

	void Application::Draw() {
		VkCommandBuffer* commandBuffer = m_VulkanEngine->BeginFrame(m_CurrentFrame, m_ImageIndex);

		if (commandBuffer == nullptr) {
			return;
		}

		BeginRenderPass(m_VulkanEngine->GetDefaultRenderPass().GetHandle(), *commandBuffer);
		m_VulkanEngine->BeginUIFrame();

		DrawFrame(*commandBuffer);
		DrawUI();

		m_VulkanEngine->EndUIFrame(*commandBuffer);
		EndRenderPass(*commandBuffer);
		m_VulkanEngine->EndFrame(*commandBuffer, m_CurrentFrame, m_ImageIndex);
		m_VulkanEngine->PresentFrame(m_CurrentFrame, m_ImageIndex);
	}

	void Application::DrawUI() {
		if (!m_Settings.uiEnabled)
			return;

		ImGui::Begin("Settings");
		ImGui::Text("Last Frame: %f ms", m_Settings.ms);
		ImGui::Text("Framerate: %.1f fps", m_Settings.frames);
		ImGui::Checkbox("Limit Framerate", &m_Settings.limitFramerate);

		p_ActiveScene->OnUIRender();
	}

	void Application::DrawFrame(const VkCommandBuffer& commandBuffer) {
		VkExtent2D swapChainExtent = m_VulkanEngine->GetSwapChain().GetSwapChainExtent();
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

		m_GlobalDescriptorSets->Bind(
			m_CurrentFrame,
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_MainPipelineLayout->GetHandle()
		);

		//RenderScene(commandBuffer, m_TexturedPipeline->GetHandle(), p_ActiveScene->RenderableObjects);
		RenderScene(commandBuffer, m_ColoredPipeline->GetHandle(), p_ActiveScene->RenderableObjects);
		RenderScene(commandBuffer, m_WireframePipeline->GetHandle(), p_ActiveScene->RenderableObjects);
		/*
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TexturedPipeline->GetHandle());
		//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ColoredPipeline->GetHandle());
		//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_WireframePipeline->GetHandle());

		for (size_t i = 0; i < p_ActiveScene->RenderableObjects.size(); i++) {
			Assets::Object* object = p_ActiveScene->RenderableObjects[i];

			object->DescriptorSets->Bind(
				m_CurrentFrame, 
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				m_MainPipelineLayout->GetHandle()
			);

			ObjectGPUData objectGPUData = ObjectGPUData();
			objectGPUData.model = object->GetModelMatrix();

			VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
			m_GPUDataBuffer->Update(m_CurrentFrame, objectBufferOffset, &objectGPUData, sizeof(ObjectGPUData));

			for (const Assets::Mesh* mesh : object->Meshes) {
				vkCmdPushConstants(
					commandBuffer,
					m_MainPipelineLayout->GetHandle(),
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
		*/
	}

	void Application::RenderScene(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline, const std::vector<Assets::Object*>& objects) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		for (size_t i = 0; i < objects.size(); i++) {
			Assets::Object* object = objects[i];

			object->DescriptorSets->Bind(
				m_CurrentFrame, 
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				m_MainPipelineLayout->GetHandle()
			);

			ObjectGPUData objectGPUData = ObjectGPUData();
			objectGPUData.model = object->GetModelMatrix();

			VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
			m_GPUDataBuffer->Update(m_CurrentFrame, objectBufferOffset, &objectGPUData, sizeof(ObjectGPUData));

			for (const Assets::Mesh* mesh : object->Meshes) {
				vkCmdPushConstants(
					commandBuffer,
					m_MainPipelineLayout->GetHandle(),
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
	}

	void Application::ProcessResize(int width, int height) {
		std::cout << "width - " << width << " height - " << height << '\n';
		m_VulkanEngine->Resize();
		p_ActiveScene->OnResize(width, height);
	}

	void Application::InitializeBuffers() {
		// GPU Data Buffer Begin
		VkDeviceSize objectBufferSize = sizeof(ObjectGPUData) * p_ActiveScene->RenderableObjects.size();
		VkDeviceSize materialsBufferSize = sizeof(Assets::MeshMaterialData) * m_Materials.size();
		VkDeviceSize sceneBufferSize = sizeof(SceneGPUData);
		
		m_GPUDataBuffer = std::make_unique<class Engine::Buffer>(
			Engine::MAX_FRAMES_IN_FLIGHT,
			*m_VulkanEngine.get(),
			objectBufferSize + materialsBufferSize + sceneBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		);
		m_GPUDataBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		m_GPUDataBuffer->NewChunk({ sizeof(ObjectGPUData), objectBufferSize});
		m_GPUDataBuffer->NewChunk({ sizeof(Assets::MeshMaterialData), materialsBufferSize});
		m_GPUDataBuffer->NewChunk({ sizeof(SceneGPUData), sceneBufferSize});

		std::vector<Assets::MeshMaterialData> meshMaterialData;

		for (const auto& material : m_Materials) {
			meshMaterialData.push_back(material.MaterialData);
		}

		Engine::BufferHelper::AppendData(
			*m_VulkanEngine.get(),
			meshMaterialData,
			*m_GPUDataBuffer.get(),
			0,
			m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize
		);
		// GPU Data Buffer End

		// Scene Geometry Buffer Begin
		VkDeviceSize bufferSize = sizeof(uint32_t) * p_ActiveScene->Indices.size() 
			+ sizeof(Assets::Vertex) * p_ActiveScene->Vertices.size();
		
		m_SceneGeometryBuffer = std::make_unique<class Engine::Buffer>(
			Engine::MAX_FRAMES_IN_FLIGHT,
			*m_VulkanEngine.get(),
			bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		);

		m_SceneGeometryBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_SceneGeometryBuffer->NewChunk({ sizeof(uint32_t), sizeof(uint32_t) * p_ActiveScene->Indices.size() });
		m_SceneGeometryBuffer->NewChunk({ sizeof(Assets::Vertex), sizeof(Assets::Vertex) * p_ActiveScene->Vertices.size() });

		Engine::BufferHelper::AppendData(
			*m_VulkanEngine.get(),
			p_ActiveScene->Indices,
			*m_SceneGeometryBuffer.get(),
			0,
			0
		);

		Engine::BufferHelper::AppendData(
			*m_VulkanEngine.get(),
			p_ActiveScene->Vertices,
			*m_SceneGeometryBuffer.get(),
			0,
			m_SceneGeometryBuffer->Chunks[INDEX_BUFFER_INDEX].ChunkSize
		);
		// Scene Geometry Buffer End
	}

	void Application::InitializeDescriptors() {
		DescriptorPoolBuilder descriptorPoolBuilder = {};
		m_DescriptorPool = descriptorPoolBuilder.AddDescriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			.AddDescriptorCount(10)
			.AddBinding()
			.AddDescriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.AddDescriptorCount(10)
			.AddBinding()
			.SetMaxSets(50)
			.Build(m_VulkanEngine->GetLogicalDevice().GetHandle());

		DescriptorSetLayoutBuild descriptorLayoutBuild = {};
		m_ObjectGPUDataDescriptorSetLayout = descriptorLayoutBuild.NewBinding(0)
			.SetDescriptorCount(1)
			.SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			.SetStage(VK_SHADER_STAGE_VERTEX_BIT)
			.SetResource(*m_GPUDataBuffer.get())
			.SetBufferSize(m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize)
			.SetBufferOffset(0)
			.Add()
			.Build(m_VulkanEngine->GetLogicalDevice().GetHandle());

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
			.Build(m_VulkanEngine->GetLogicalDevice().GetHandle());

		// Renderable Objects Descriptor Sets Begin
		for (size_t i = 0; i < p_ActiveScene->RenderableObjects.size(); i++) {
			Assets::Object* renderableObject = p_ActiveScene->RenderableObjects[i];
			VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;

			m_ObjectGPUDataDescriptorSetLayout->UpdateOffset(0, objectBufferOffset);

			renderableObject->DescriptorSets = std::make_unique<class Engine::DescriptorSets>(
				m_VulkanEngine->GetLogicalDevice().GetHandle(),
				m_DescriptorPool->GetHandle(),
				*m_ObjectGPUDataDescriptorSetLayout.get(),
				0,
				1
			);
		}
		// Renderable Objects Descriptor Sets End 

		// Global Descriptor Sets Begin
		m_GlobalDescriptorSets = std::make_unique<class DescriptorSets>(
			m_VulkanEngine->GetLogicalDevice().GetHandle(),
			m_DescriptorPool->GetHandle(),
			*m_GlobalDescriptorSetLayout.get(),
			1,
			1
		);
		// Global Descriptor Sets End
	}

	void Application::CreatePipelineLayouts() {
		PipelineLayoutBuilder pipelineLayoutBuilder = PipelineLayoutBuilder();
		
		VkPushConstantRange mainPipelinePushConstant = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) };

		m_MainPipelineLayout = pipelineLayoutBuilder.AddDescriptorSetLayout(m_ObjectGPUDataDescriptorSetLayout->GetHandle())
			.AddDescriptorSetLayout(m_GlobalDescriptorSetLayout->GetHandle())
			.AddPushConstant(mainPipelinePushConstant)
			.BuildPipelineLayout(m_VulkanEngine->GetLogicalDevice().GetHandle());
	}

	void Application::CreateGraphicsPipelines() {

		Assets::VertexShader defaultVertexShader = Assets::VertexShader("Default Vertex Shader", "C:/Users/Felipe/Documents/current_projects/VulkanApplication/Assets/Shaders/default_vert.spv");
		Assets::FragmentShader texturedFragmentShader = Assets::FragmentShader("Textured Fragment Shader", "C:/Users/Felipe/Documents/current_projects/VulkanApplication/Assets/Shaders/textured_frag.spv");

		PipelineBuilder pipelineBuilder = PipelineBuilder();
		m_TexturedPipeline = pipelineBuilder.AddVertexShader(defaultVertexShader)
			.AddFragmentShader(texturedFragmentShader)
			.AddRenderPass(m_VulkanEngine->GetDefaultRenderPass().GetHandle())
			.AddPipelineLayout(*m_MainPipelineLayout)
			.BuildGraphicsPipeline(*m_VulkanEngine.get());

		Assets::FragmentShader wireframeFragShader = Assets::FragmentShader("Wireframe Fragment Shader", "./Assets/Shaders/wireframe_frag.spv");
		wireframeFragShader.PolygonMode = Assets::FragmentShader::Polygon::LINE;

		m_WireframePipeline = pipelineBuilder.AddVertexShader(defaultVertexShader)
			.AddFragmentShader(wireframeFragShader)
			.AddRenderPass(m_VulkanEngine->GetDefaultRenderPass().GetHandle())
			.AddPipelineLayout(*m_MainPipelineLayout)
			.BuildGraphicsPipeline(*m_VulkanEngine.get());

		Assets::FragmentShader coloredFragShader = Assets::FragmentShader("Colored Fragment Shader", "./Assets/Shaders/colored_frag.spv");

		m_ColoredPipeline = pipelineBuilder.AddVertexShader(defaultVertexShader)
			.AddFragmentShader(coloredFragShader)
			.AddRenderPass(m_VulkanEngine->GetDefaultRenderPass().GetHandle())
			.AddPipelineLayout(*m_MainPipelineLayout)
			.BuildGraphicsPipeline(*m_VulkanEngine.get());
	}

	void Application::BeginRenderPass(const VkRenderPass& renderPass, VkCommandBuffer& commandBuffer) {
		VkExtent2D swapChainExtent = m_VulkanEngine->GetSwapChain().GetSwapChainExtent();
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_VulkanEngine->GetDefaultRenderPass().GetHandle();
		renderPassBeginInfo.framebuffer = m_VulkanEngine->GetFramebuffer(m_ImageIndex);
		renderPassBeginInfo.renderArea.offset = {0, 0};
		renderPassBeginInfo.renderArea.extent = swapChainExtent;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Application::EndRenderPass(VkCommandBuffer& commandBuffer) {
		vkCmdEndRenderPass(commandBuffer);
	}
}
