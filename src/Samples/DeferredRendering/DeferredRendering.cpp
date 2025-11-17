#include <iostream>

#include "../../src/Core/VulkanHeader.h"
#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/RenderTarget.h"
#include "../../src/Core/Profiler.h"
#include "../../src/Core/ResourceManager.h"
#include "../../src/Core/SceneComponents.h"

#include "../../src/Utils/ModelLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

constexpr int MAX_MODELS = 20;
constexpr int MAX_LIGHTS = 20;

class DeferredRendering : public Application::IScene {
public:
	DeferredRendering() {
		settings.Title = "DeferredRendering.exe";
		settings.Width = 1600;
		settings.Height = 900;
		settings.uiEnabled = true;
	};

	virtual void StartUp()																			override;
	virtual void CleanUp()																			override;
	virtual void Update(const float constantT, const float deltaT, InputSystem::Input& input)		override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer)		override;
	virtual void RenderUI()																			override;
	virtual void Resize(uint32_t width, uint32_t height)											override;

	// For comparison
	struct ForwardRenderingPass {
		Graphics::PipelineState PSO		= {};
		Graphics::Shader VertexShader	= {};
		Graphics::Shader FragShader		= {};
		std::unique_ptr<Graphics::OffscreenRenderTarget> RenderTarget;
	} ForwardPass;

	struct DefererredRenderingPass {
		Graphics::PipelineState PSO		= {};
		Graphics::Shader VertexShader	= {};
		Graphics::Shader FragShader		= {};
		std::unique_ptr<Graphics::MultiAttachmentRenderTarget> RenderTarget;
	} DeferredPass;

	struct SceneData {
		alignas(16) glm::mat4 Projection;
		alignas(16) glm::mat4 View;
		alignas(16) glm::vec4 extra[6];
		alignas(16) glm::vec4 ViewPosition;
		alignas(4) int TotalLights;
		int extra1;
		int extra2;
		int extra3;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model;
		alignas(4) uint32_t MaterialIndex;
	} SamplePushConstants;

	const glm::vec3 InitialCameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= -90.0f;
	const float InitialCameraPitch	= 0.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

	std::array<std::shared_ptr<Assets::Model>, MAX_MODELS> m_Models;
	std::array<Scene::LightComponent, MAX_LIGHTS> m_Lights;

	size_t TotalModels = 0;
	size_t TotalLights = 0;

	Graphics::Buffer m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT] = {};
	Graphics::Buffer m_LightBuffer = {};

	Graphics::InputLayout m_PipelineInputLayout = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_Set = { VK_NULL_HANDLE };

	bool m_DeferredRenderingEnabled = false;
private:
	void InitializeForwardPassResources();
	void InitializeDeferredPassResources();

	void RenderForward(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void RenderDeferred(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);

	void AddLight();
	void RemoveLight();
};

void DeferredRendering::AddLight() {
	if (TotalLights + 1 > MAX_LIGHTS)
		return;

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
	float lightIntensity = 1.0f;

	m_Lights[TotalLights].ambient = 0.1f;
	m_Lights[TotalLights].diffuse = 1.0f;
	m_Lights[TotalLights].specular = 1.0f;
	m_Lights[TotalLights].position = glm::vec4(x, y, z, 0.0f);
	m_Lights[TotalLights].color = glm::vec4(r, g, b, lightIntensity);
	
	TotalLights++;

	SampleSceneData.TotalLights = TotalLights;
}

void DeferredRendering::RemoveLight() {
	if (TotalLights == 0)
		return;

	TotalLights--;

	SampleSceneData.TotalLights = TotalLights;
}


void DeferredRendering::InitializeForwardPassResources() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	ForwardPass.RenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, ForwardPass.VertexShader, "../src/Samples/DeferredRendering/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, ForwardPass.FragShader, "../src/Samples/DeferredRendering/fragment.glsl");

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Forward Rendering - Phong";
	desc.vertexShader = &ForwardPass.VertexShader;
	desc.fragmentShader = &ForwardPass.FragShader;
	desc.psoInputLayout.push_back(m_PipelineInputLayout);

	gfxDevice->CreatePipelineState(desc, ForwardPass.PSO, *ForwardPass.RenderTarget.get());
}

void DeferredRendering::InitializeDeferredPassResources() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	const std::vector<Graphics::MultiAttachmentRenderTarget::AttachmentDescription> attachments = {
		{ .Samples = gfxDevice->GetMsaaSamples(), .ImageFormat = Graphics::Format::R16G16B16A16_FLOAT },	// Position
		{ .Samples = gfxDevice->GetMsaaSamples(), .ImageFormat = Graphics::Format::R16G16B16A16_FLOAT },	// Normal
		{ .Samples = gfxDevice->GetMsaaSamples(), .ImageFormat = Graphics::Format::R16G16B16A16_FLOAT }		// AlbedoSpec 
	};

	DeferredPass.RenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
		m_ScreenWidth,
		m_ScreenHeight,
		attachments
	);
}

void DeferredRendering::StartUp() {

	m_ScreenWidth = settings.Width;
	m_ScreenHeight = settings.Height;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

	for (size_t i = 0; i < Graphics::FRAMES_IN_FLIGHT; ++i) {
		m_SceneBuffer[i] = gfxDevice->CreateBuffer(sizeof(SceneData));
	}

	m_LightBuffer = gfxDevice->CreateBuffer(sizeof(Scene::LightComponent) * MAX_LIGHTS);

	m_Models[TotalModels] = ModelLoader::LoadModel(ModelType::QUAD);
	m_Models[TotalModels]->Transformations.translation		= glm::vec3(0.0f, -0.51f, 0.0f);
	m_Models[TotalModels]->Transformations.rotation.x		= 90.0f;
	m_Models[TotalModels]->Transformations.scaleHandler		= 20.0f;
	m_Models[TotalModels]->ModelIndex						= TotalModels;
	
	TotalModels++;

	const int maxRow = 5;
	const int maxColumn = 3;

	const float initialX = 0.0f - (maxRow / 2.0f);
	const float initialZ = 0.0f - (maxColumn / 2.0f);

	const float offsetIncrease = 0.5f;

	float offsetX = 0.0f;
	float offsetZ = 0.0f;

	uint32_t originalModelIndex = 0;

	for (size_t i = 0; i < maxRow; ++i) {

		float x = i + initialX + offsetX;

		for (size_t j = 0; j < maxColumn; j++) {

			float z = j + initialZ + offsetZ;

			if (i == 0 && j == 0) {
				m_Models[TotalModels] = ModelLoader::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj");
				ModelLoader::FlipModelUvVertically(*m_Models[TotalModels].get());
				originalModelIndex = TotalModels;
			}
			else {
				m_Models[TotalModels] = ModelLoader::DuplicateModel(m_Models[originalModelIndex]);
			}

			m_Models[TotalModels]->Transformations.scaleHandler		= 0.3f;
			m_Models[TotalModels]->Transformations.translation		= glm::vec3(x, 0.6f, z);
			m_Models[TotalModels]->FlipUvVertically					= true;
			m_Models[TotalModels]->ModelIndex						= TotalModels;
			
			TotalModels++;

			offsetZ += offsetIncrease;
		}

		offsetZ = 0.0f;
		offsetX += offsetIncrease;
	}

	for (size_t i = 0; i < 5; ++i) {
		float x = 0.0f + static_cast<float>(i);
		float y = 2.0f;
		float z = 0.0f + static_cast<float>(i);

		float r = 1.0f;
		float g = 1.0f;
		float b = 1.0f;
		float lightIntensity = 1.0f;

		m_Lights[TotalLights].ambient = 0.1f;
		m_Lights[TotalLights].diffuse = 1.0f;
		m_Lights[TotalLights].specular = 1.0f;
		m_Lights[TotalLights].position = glm::vec4(x, y, z, 0.0f);
		m_Lights[TotalLights].color = glm::vec4(r, g, b, lightIntensity);
		
		TotalLights++;
	}

	ResourceManager* rm = ResourceManager::Get();

	const uint32_t totalLoadedTextures = static_cast<uint32_t>(rm->GetTotalTextures());

	m_PipelineInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			// Scene GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },										// Material GPU Data
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalLoadedTextures, VK_SHADER_STAGE_FRAGMENT_BIT},				// Textures 
			{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}										// Light GPU Data 
		}
	};

	InitializeForwardPassResources();
	InitializeDeferredPassResources();

	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, m_PipelineInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);

		gfxDevice->WriteDescriptor(m_PipelineInputLayout.bindings[0], m_Set[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(m_PipelineInputLayout.bindings[1], m_Set[i], rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(m_PipelineInputLayout.bindings[2], m_Set[i], rm->GetTextures());
		gfxDevice->WriteDescriptor(m_PipelineInputLayout.bindings[3], m_Set[i], m_LightBuffer);
	}
}

void DeferredRendering::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	ForwardPass.RenderTarget.reset();

	gfxDevice->DestroyShader(ForwardPass.VertexShader);
	gfxDevice->DestroyShader(ForwardPass.FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(ForwardPass.PSO);
	
	DeferredPass.RenderTarget.reset();
}

void DeferredRendering::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("DeferredRendering::Update");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	m_Camera.OnUpdate(deltaT, input);

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
	SampleSceneData.ViewPosition	= glm::vec4(m_Camera.Position, 1.0f);
	SampleSceneData.TotalLights		= TotalLights;

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], &SampleSceneData);
	gfxDevice->UpdateBuffer(m_LightBuffer, m_Lights.data());
}

void DeferredRendering::RenderForward(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("DeferredRendering::RenderForward");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	ForwardPass.RenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(m_Set[currentFrame], commandBuffer, ForwardPass.PSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ForwardPass.PSO.pipeline);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {

		Assets::Model& Model = *m_Models[ModelIndex].get();

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		SamplePushConstants.Model = Model.GetModelMatrix();

		for (const auto& Mesh: Model.Meshes) {

			SamplePushConstants.MaterialIndex = Mesh.MaterialIndex;

			vkCmdPushConstants(commandBuffer, ForwardPass.PSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &SamplePushConstants);

			vkCmdDrawIndexed(
				commandBuffer, 
				static_cast<uint32_t>(Mesh.Indices.size()), 
				1, 
				static_cast<uint32_t>(Mesh.IndexOffset), 
				static_cast<int32_t>(Mesh.VertexOffset),
				0);
		}
	}

	ForwardPass.RenderTarget->End(commandBuffer);

	ForwardPass.RenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(ForwardPass.RenderTarget->GetColorBuffer());

}

void DeferredRendering::RenderDeferred(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("DeferredRendering::RenderDeferred");
}

void DeferredRendering::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("DeferredRendering::RenderScene");

	if (m_DeferredRenderingEnabled) {
		RenderDeferred(currentFrame, commandBuffer);
	}
	else {
		RenderForward(currentFrame, commandBuffer);
	}
}

void DeferredRendering::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	ImGui::Checkbox("Deferred Rendering Enabled", &m_DeferredRenderingEnabled);

	m_Camera.OnUIRender("Main Camera - Settings");

	if (ImGui::TreeNode("Lights")) {
		for (size_t LightIndex = 0; LightIndex < TotalLights; ++LightIndex) {
			std::string LightId = "Light ";
			LightId = LightId.append(std::to_string(LightIndex));
	
			if (ImGui::TreeNode(LightId.c_str())) {
				ImGui::DragFloat4("Light Position", (float*)&m_Lights[LightIndex].position, 0.02f, -20.0f, 20.0f);
				ImGui::ColorPicker4("Light Color", (float*)&m_Lights[LightIndex].color);
				ImGui::DragFloat("Light Intensity", (float*)&m_Lights[LightIndex].color.a, 0.02f, 0.0f, 1.0f);

				ImGui::TreePop();
			}
		}

		if (ImGui::Button("Add Light")) {
			AddLight();
		}

		if (ImGui::Button("Remove Light")) {
			RemoveLight();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Models")) {
		for (size_t ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {
			m_Models[ModelIndex]->OnUIRender();
		}
		
		ImGui::TreePop();
	}
}

void DeferredRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);

	ForwardPass.RenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
	DeferredPass.RenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

RUN_APPLICATION(DeferredRendering);