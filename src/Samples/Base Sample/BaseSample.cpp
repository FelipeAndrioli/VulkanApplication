#include <iostream>

#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/RenderTarget.h"
#include "../../src/Core/Profiler.h"

#include "../../src/Core/VulkanHeader.h"

#include "../../src/Utils/ModelLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

constexpr int TOTAL_MODELS = 3;

class BaseSample : public Application::IScene {
public:
	BaseSample() {
		settings.Title = "BaseSample.exe";
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

	struct SceneData {
		alignas(16) glm::mat4 Projection;
		alignas(16) glm::mat4 View;
		alignas(16) glm::vec4 LightPosition;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model;
	} SamplePushConstants;

	const glm::vec3 InitialCameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= -90.0f;
	const float InitialCameraPitch	= 0.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

	std::array<std::shared_ptr<Assets::Model>, TOTAL_MODELS> m_Models;

	size_t TotalModels = 0;

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_OffscreenRenderTarget;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::Buffer m_SceneBuffer = {};

	Graphics::PipelineState m_PSO = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_Set = { VK_NULL_HANDLE };

	glm::vec4 m_LightPosition = glm::vec4(1.0f);

	float m_OrbitalLightSpeed = 0.5f;
	float m_OrbitalLightDisplacement = 3.0f;
	bool m_OrbitateLight = false;
private:
	void LoadSampleModel();
	void DeleteSampleModel();
};

void BaseSample::StartUp() {

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

	m_OffscreenRenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/Base Sample/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/Base Sample/fragment.glsl");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(SceneData));

	Graphics::InputLayout inputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },			
		}
	};

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Parallax Mapping";
	desc.vertexShader = &m_VertexShader;
	desc.fragmentShader = &m_FragShader;
	desc.psoInputLayout.push_back(inputLayout);

	gfxDevice->CreatePipelineState(desc, m_PSO, *m_OffscreenRenderTarget.get());
	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, inputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[0], m_Set[i], m_SceneBuffer);
	}

	m_Models[TotalModels] = ModelLoader::LoadModel(ModelType::QUAD);
	m_Models[TotalModels]->Transformations.translation.y = -0.51f;
	m_Models[TotalModels]->Transformations.rotation.x = 90.0f;
	m_Models[TotalModels]->Transformations.scaleHandler = 20.0f;
	m_Models[TotalModels]->ModelIndex = TotalModels;

	TotalModels++;
}

void BaseSample::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(m_PSO);
}

void BaseSample::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("BaseSample::Update");

	m_Camera.OnUpdate(deltaT, input);

	if (input.Keys[GLFW_KEY_L].IsPressed) {
		LoadSampleModel();
	}
	else if (input.Keys[GLFW_KEY_K].IsPressed) {
		DeleteSampleModel();
	}
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	if (m_OrbitateLight) {
		m_LightPosition.x = glm::sin(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
		m_LightPosition.z = glm::cos(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
	}

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
	SampleSceneData.LightPosition	= m_LightPosition;

	gfxDevice->UpdateBuffer(m_SceneBuffer, &SampleSceneData);
}

void BaseSample::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("BaseSample::RenderScene");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(m_Set[currentFrame], commandBuffer, m_PSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {

		Assets::Model& Model = *m_Models[ModelIndex].get();

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		SamplePushConstants.Model = Model.GetModelMatrix();

		vkCmdPushConstants(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &SamplePushConstants);

		for (const auto& Mesh: Model.Meshes) {
			vkCmdDrawIndexed(
				commandBuffer, 
				static_cast<uint32_t>(Mesh.Indices.size()), 
				1, 
				static_cast<uint32_t>(Mesh.IndexOffset), 
				static_cast<int32_t>(Mesh.VertexOffset),
				0);
		}
	}

	m_OffscreenRenderTarget->End(commandBuffer);

	m_OffscreenRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_OffscreenRenderTarget->GetColorBuffer());
}

void BaseSample::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

	ImGui::Checkbox("Orbitate Light",				&m_OrbitateLight);
	ImGui::DragFloat("Light Orbital Speed",			&m_OrbitalLightSpeed, 0.02f, 0.0f, 3.0f);
	ImGui::DragFloat("Light Orbital Displacement",	&m_OrbitalLightDisplacement, 0.02f, 0.0f, 9.0f);
	ImGui::DragFloat4("Light Position",				(float*)&m_LightPosition, 0.02f, -20.0f, 20.0f);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {
		m_Models[ModelIndex]->OnUIRender();
	}
}

void BaseSample::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

void BaseSample::LoadSampleModel() {
	if (TotalModels + 1 > TOTAL_MODELS)
		return;

	m_Models[TotalModels] = ModelLoader::LoadModel(ModelType::CUBE);
	m_Models[TotalModels]->ModelIndex = TotalModels;
	
	TotalModels++;
}

void BaseSample::DeleteSampleModel() {
	if (TotalModels == 0)
		return;

	TotalModels--;
}

//RUN_APPLICATION(BaseSample);