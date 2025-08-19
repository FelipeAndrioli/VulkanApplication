#include <iostream>

#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/RenderTarget.h"
#include "../../src/Core/Profiler.h"
#include "../../src/Core/ResourceManager.h"

#include "../../src/Core/VulkanHeader.h"

#include "../../src/Utils/ModelLoader.h"
#include "../../src/Utils/TextureLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

constexpr int TOTAL_MODELS = 3;

class ParallaxMapping : public Application::IScene {
public:
	ParallaxMapping() {
		settings.Title = "ParallaxMapping.exe";
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
		alignas(16) glm::mat4 Projection	= glm::mat4(1.0f);
		alignas(16) glm::mat4 View			= glm::mat4(1.0f);
		alignas(16) glm::vec4 LightPosition = glm::vec4(1.0f);
		alignas(16) glm::vec4 ViewPosition	= glm::vec4(1.0f);
		alignas(4) float HeightScale		= 0.100f;
		alignas(4) float LayerSize			= 4.0f;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model = glm::mat4(1.0f);
		alignas(4) int Flags		= 0;
		alignas(4) int DebugFlags	= 0;
		alignas(4) int MinLayers	= 8;
		alignas(4) int MaxLayers	= 100;
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

	Graphics::PipelineStateDescription m_PSODesc = {};
	Graphics::InputLayout m_PSOInputLayout = {};
	Graphics::PipelineState m_PSO = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_Set = { VK_NULL_HANDLE };

	glm::vec4 m_LightPosition = glm::vec4(1.0f);

	float m_OrbitalLightSpeed				= 0.5f;
	float m_OrbitalLightDisplacement		= 3.0f;
	bool m_OrbitateLight					= false;
	bool m_ParallaxMappingEnabled			= true;
	bool m_DiscardOversampledFragments		= true;
	bool m_OffsetLimiting					= true;
	bool m_SteepParallaxMapping				= true;
	bool m_SteepParallaxOcclusionMapping	= true;

	bool m_DebugEnabled = false;
	bool m_DebugRenderTangent = false;
	bool m_DebugRenderBiTangent = false;
	bool m_DebugRenderMeshNormal = false;
	bool m_DebugRenderTextureNormal = false;

	uint32_t m_DiffuseTextureIndex		= 0;
	uint32_t m_NormalTextureIndex		= 0;
	uint32_t m_DisplacementTextureIndex = 0;
private:
	void LoadPipeline();
	void LoadAssets();
};

void ParallaxMapping::LoadAssets() {

	m_Models[TotalModels] = ModelLoader::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/plane/plane.gltf");
	m_Models[TotalModels]->Transformations.scaleHandler = 0.5f;
	m_Models[TotalModels]->ModelIndex = TotalModels;

	TotalModels++;

	m_Models[TotalModels] = ModelLoader::LoadModel(ModelType::QUAD);
	m_Models[TotalModels]->Transformations.rotation.x = 90.0f;
	m_Models[TotalModels]->Transformations.translation.x = 5.0f;
	m_Models[TotalModels]->Transformations.scaleHandler = 5.0f;
	m_Models[TotalModels]->ModelIndex = TotalModels;

	TotalModels++;

	ResourceManager* rm = ResourceManager::Get();

	m_DiffuseTextureIndex		= rm->AddTexture(TextureLoader::LoadTexture("../src/Samples/Parallax Mapping/bricks2.jpg",			Texture::TextureType::DIFFUSE,		false, false));
	m_NormalTextureIndex		= rm->AddTexture(TextureLoader::LoadTexture("../src/Samples/Parallax Mapping/bricks2_normal.jpg",	Texture::TextureType::NORMAL,		false, false));
	m_DisplacementTextureIndex	= rm->AddTexture(TextureLoader::LoadTexture("../src/Samples/Parallax Mapping/bricks2_disp.jpg",		Texture::TextureType::DISPLACEMENT, false, false));
}

void ParallaxMapping::LoadPipeline() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/Parallax Mapping/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/Parallax Mapping/fragment.glsl");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(SceneData));

	m_PSOInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },				// Scene Data
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },		// Diffuse Texture
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },		// Normal Texture
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },		// Displacement Texture
		}
	};

	m_PSODesc.Name = "Parallax Mapping";
	m_PSODesc.vertexShader = &m_VertexShader;
	m_PSODesc.fragmentShader = &m_FragShader;
	m_PSODesc.psoInputLayout.push_back(m_PSOInputLayout);

	gfxDevice->CreatePipelineState(m_PSODesc, m_PSO, *m_OffscreenRenderTarget.get());
	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, m_PSOInputLayout.bindings);

	ResourceManager* rm = ResourceManager::Get();

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[0], m_Set[i], m_SceneBuffer);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[1], m_Set[i], rm->GetTextures()[m_DiffuseTextureIndex]);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[2], m_Set[i], rm->GetTextures()[m_NormalTextureIndex]);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[3], m_Set[i], rm->GetTextures()[m_DisplacementTextureIndex]);
	}
}

void ParallaxMapping::StartUp() {

	assert(sizeof(SceneData) <= 256 && "UBO Size cannot exceed 256 bytes!");

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

	m_OffscreenRenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

	m_LightPosition = glm::vec4(0.0f, 0.0f, 2.2f, 1.0f);

	LoadAssets();
	LoadPipeline();
}

void ParallaxMapping::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(m_PSO);
}

void ParallaxMapping::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("ParallaxMapping::Update");

	m_Camera.OnUpdate(deltaT, input);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {
		m_Models[ModelIndex]->OnUpdate(deltaT);
	}

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	if (m_OrbitateLight) {
		m_LightPosition.x = glm::sin(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
		m_LightPosition.z = glm::cos(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
	}

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
	SampleSceneData.LightPosition	= m_LightPosition;
	SampleSceneData.ViewPosition	= glm::vec4(m_Camera.Position, 1.0);

	gfxDevice->UpdateBuffer(m_SceneBuffer, &SampleSceneData);
}

void ParallaxMapping::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("ParallaxMapping::RenderScene");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(m_Set[currentFrame], commandBuffer, m_PSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {

		Assets::Model& Model = *m_Models[ModelIndex].get();

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		SamplePushConstants.Model	= Model.GetModelMatrix();
		SamplePushConstants.Flags	= ((Model.FlipUvVertically << 5)
			| (m_SteepParallaxOcclusionMapping << 4) 
			| (m_SteepParallaxMapping << 3) 
			| (m_OffsetLimiting << 2) 
			| (m_DiscardOversampledFragments << 1) 
			| m_ParallaxMappingEnabled);
		SamplePushConstants.DebugFlags = (
			(m_DebugRenderTangent << 4)
			| (m_DebugRenderBiTangent << 3)
			| (m_DebugRenderTextureNormal << 2)
			| (m_DebugRenderMeshNormal << 1)
			| (m_DebugEnabled));

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

void ParallaxMapping::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

	ImGui::Checkbox("Debug Enabled", &m_DebugEnabled);

	ImGui::Checkbox("Debug - Render Tangent",			&m_DebugRenderTangent);

	if (m_DebugRenderTangent) {
		m_DebugRenderBiTangent		= false;
		m_DebugRenderMeshNormal		= false;
		m_DebugRenderTextureNormal	= false;
	}

	ImGui::Checkbox("Debug - Render BiTangent",			&m_DebugRenderBiTangent);

	if (m_DebugRenderBiTangent) {
		m_DebugRenderTangent		= false;
		m_DebugRenderMeshNormal		= false;
		m_DebugRenderTextureNormal	= false;
	}

	ImGui::Checkbox("Debug - Render Mesh Normal",		&m_DebugRenderMeshNormal);

	if (m_DebugRenderMeshNormal) {
		m_DebugRenderBiTangent		= false;
		m_DebugRenderTangent		= false;
		m_DebugRenderTextureNormal	= false;
	}

	ImGui::Checkbox("Debug - Render Texture Normal",	&m_DebugRenderTextureNormal);

	if (m_DebugRenderTextureNormal) {
		m_DebugRenderMeshNormal		= false;
		m_DebugRenderBiTangent		= false;
		m_DebugRenderTangent		= false;
	}

	ImGui::Checkbox("Parallax Mapping Enabled",			&m_ParallaxMappingEnabled);
	ImGui::Checkbox("Discard Oversampled Fragments",	&m_DiscardOversampledFragments);
	ImGui::Checkbox("Offset Limiting",					&m_OffsetLimiting);
	ImGui::Checkbox("Orbitate Light",					&m_OrbitateLight);
	ImGui::Checkbox("Steep Parallax Mapping",			&m_SteepParallaxMapping);
	ImGui::Checkbox("Steep Parallax Occlusion Mapping",	&m_SteepParallaxOcclusionMapping);
	ImGui::DragFloat("Light Orbital Speed",				&m_OrbitalLightSpeed, 0.02f, 0.0f, 3.0f);
	ImGui::DragFloat("Light Orbital Displacement",		&m_OrbitalLightDisplacement, 0.02f, 0.0f, 9.0f);
	ImGui::DragFloat4("Light Position",					(float*)&m_LightPosition, 0.02f, -20.0f, 20.0f);
	
	ImGui::DragFloat("Height Scale",					&SampleSceneData.HeightScale, 0.02, 0.0f, 1.0f);
	ImGui::DragFloat("Layer Size",						&SampleSceneData.LayerSize, 0.02, 0.0f, 10.0f);
	ImGui::DragInt("Min Layers",						&SamplePushConstants.MinLayers, 1.0f, 0, 200);
	ImGui::DragInt("Max Layers",						&SamplePushConstants.MaxLayers, 1.0f, 0, 200);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {
		m_Models[ModelIndex]->OnUIRender();
	}
}

void ParallaxMapping::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

RUN_APPLICATION(ParallaxMapping);