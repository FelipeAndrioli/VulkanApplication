#include <iostream>

#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/RenderTarget.h"
#include "../../src/Core/Profiler.h"
#include "../../src/Core/ResourceManager.h"

#include "../../src/Core/VulkanHeader.h"

#include "../../src/Utils/ModelLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

constexpr int TOTAL_MODELS = 3;

class AmbientOcclusion : public Application::IScene {
public:
	AmbientOcclusion() {
		settings.Title = "AmbientOcclusion.exe";
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

	struct SceneUBOData {
        alignas(16) glm::mat4 Extra;
		alignas(16) glm::mat4 Projection;
		alignas(16) glm::mat4 View;
		alignas(16) glm::vec4 Light;
        alignas(16) glm::vec4 Extra1[3];
	} SampleSceneUBOData;

	struct PushConstants {
		alignas(16) glm::mat4 Model;
        alignas(4) int MaterialIndex;
	} SamplePushConstants;

    struct PostProcessUBO {
        alignas(16) glm::vec4 Extra[15];
        alignas(4) float Gamma = 2.2f;
        alignas(4) float Extra1 = 0.0f;
        alignas(4) float Extra2 = 0.0f;
        alignas(4) float Extra3 = 0.0f;
    } PostProcessUBOData;

	const glm::vec3 InitialCameraPosition = glm::vec3(-10.0f, -3.5f, -0.2f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= 1.0f;
	const float InitialCameraPitch	= -10.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

    bool m_RenderPostEffects = true;

	std::array<std::shared_ptr<Assets::Model>, TOTAL_MODELS> m_Models;

	size_t TotalModels = 0;

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_OffscreenRenderTarget;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::Buffer m_SceneBuffer = {};

	Graphics::PipelineState m_PSO = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_Set = { VK_NULL_HANDLE };

    std::unique_ptr<Graphics::PostEffectsRenderTarget> m_PostEffectsRenderTarget;
    Graphics::Shader m_PostEffectsVertexShader = {};
    Graphics::Shader m_PostEffectsFragmentShader = {};
    Graphics::PipelineState m_PostEffectsPSO = {};
    Graphics::Buffer m_PostEffectsBuffer = {};
    Graphics::InputLayout m_PostEffectsInputLayout = {};

	VkDescriptorSetLayout m_PostEffectsSetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_PostEffectsSet = { VK_NULL_HANDLE };

	glm::vec4 m_Light = glm::vec4(0.1f, 1.0f, 0.0f, 0.1f);
};

void AmbientOcclusion::StartUp() {

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;
    
    m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

    m_Models[TotalModels] = ModelLoader::LoadModel("C:/Users/felip/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj");
    m_Models[TotalModels]->Transformations.scaleHandler = 0.008f;
    m_Models[TotalModels]->ModelIndex = TotalModels;

    TotalModels++;

	m_OffscreenRenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
    ResourceManager* rm = ResourceManager::Get();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/AmbientOcclusion/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/AmbientOcclusion/fragment.glsl");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(SceneUBOData));

	Graphics::InputLayout inputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(rm->GetTextures().size()), VK_SHADER_STAGE_FRAGMENT_BIT },			
		}
	};

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Lighting";
	desc.vertexShader = &m_VertexShader;
	desc.fragmentShader = &m_FragShader;
	desc.psoInputLayout.push_back(inputLayout);

	gfxDevice->CreatePipelineState(desc, m_PSO, *m_OffscreenRenderTarget.get());
	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, inputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[0], m_Set[i], m_SceneBuffer);
		gfxDevice->WriteDescriptor(inputLayout.bindings[1], m_Set[i], rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(inputLayout.bindings[2], m_Set[i], rm->GetTextures());
	}

    m_PostEffectsRenderTarget = std::make_unique<Graphics::PostEffectsRenderTarget>(m_ScreenWidth, m_ScreenHeight);

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_PostEffectsVertexShader, "../src/Samples/AmbientOcclusion/post_process_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_PostEffectsFragmentShader, "../src/Samples/AmbientOcclusion/post_process_fragment.glsl");

    m_PostEffectsBuffer = gfxDevice->CreateBuffer(sizeof(PostProcessUBOData));

    m_PostEffectsInputLayout = {
        .pushConstants = {},
        .bindings = {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        }
    };

	Graphics::PipelineStateDescription postProcessDesc = {};
    postProcessDesc.Name = "Post Processing";
    postProcessDesc.vertexShader = &m_PostEffectsVertexShader;
    postProcessDesc.fragmentShader= &m_PostEffectsFragmentShader;
    postProcessDesc.noVertex = true;
    postProcessDesc.cullMode= VK_CULL_MODE_NONE;
    postProcessDesc.psoInputLayout.push_back(m_PostEffectsInputLayout);

    gfxDevice->CreatePipelineState(postProcessDesc, m_PostEffectsPSO, *m_PostEffectsRenderTarget.get());
    gfxDevice->CreateDescriptorSetLayout(m_PostEffectsSetLayout, m_PostEffectsInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_PostEffectsSetLayout, m_PostEffectsSet[i]);
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[0], m_PostEffectsSet[i], m_PostEffectsBuffer);
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[1], m_PostEffectsSet[i], m_OffscreenRenderTarget->GetColorBuffer());
    }
}

void AmbientOcclusion::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(m_PSO);

    m_PostEffectsRenderTarget.reset();

    gfxDevice->DestroyShader(m_PostEffectsVertexShader);
    gfxDevice->DestroyShader(m_PostEffectsFragmentShader);
	gfxDevice->DestroyDescriptorSetLayout(m_PostEffectsSetLayout);
	gfxDevice->DestroyPipeline(m_PostEffectsPSO);
}

void AmbientOcclusion::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("AmbientOcclusion::Update");

	m_Camera.OnUpdate(deltaT, input);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	SampleSceneUBOData.Projection	= m_Camera.ProjectionMatrix;
	SampleSceneUBOData.View			= m_Camera.ViewMatrix;
	SampleSceneUBOData.Light        = m_Light;

	gfxDevice->UpdateBuffer(m_SceneBuffer, &SampleSceneUBOData);
	gfxDevice->UpdateBuffer(m_PostEffectsBuffer, &PostProcessUBOData);
}

void AmbientOcclusion::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("AmbientOcclusion::RenderScene");

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

		for (const auto& Mesh: Model.Meshes) {
            SamplePushConstants.MaterialIndex = Mesh.MaterialIndex;

            vkCmdPushConstants(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &SamplePushConstants);

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

    if (m_RenderPostEffects) {
        m_PostEffectsRenderTarget->Begin(commandBuffer);

        gfxDevice->BindDescriptorSet(m_PostEffectsSet[currentFrame], commandBuffer, m_PostEffectsPSO.pipelineLayout, 0, 1);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostEffectsPSO.pipeline);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);

        m_PostEffectsRenderTarget->End(commandBuffer);

        m_PostEffectsRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_PostEffectsRenderTarget->GetColorBuffer());
    } else {
        m_OffscreenRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_OffscreenRenderTarget->GetColorBuffer());
    }
}

void AmbientOcclusion::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

    ImGui::DragFloat4("Light Direction", (float*)&m_Light, 0.02f, -20.0f, 20.0f);
    ImGui::DragFloat("Light Intensity", &m_Light.w, 0.002f, 0.0f, 1.0f);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {
		m_Models[ModelIndex]->OnUIRender();
	}

    ImGui::SeparatorText("Scene Post Effects");
    ImGui::Checkbox("Render Post Effects", &m_RenderPostEffects);
    ImGui::DragFloat("Gamma Correction", &PostProcessUBOData.Gamma, 0.002f, 0.0f, 5.0f);
}

void AmbientOcclusion::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
    m_PostEffectsRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[1], m_PostEffectsSet[i], m_OffscreenRenderTarget->GetColorBuffer());
    }
}

RUN_APPLICATION(AmbientOcclusion);
