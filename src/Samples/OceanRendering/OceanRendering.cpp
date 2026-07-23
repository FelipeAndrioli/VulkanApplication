#include <iostream>

#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/RenderTarget.h"
#include "../../src/Core/Profiler.h"

#include "../../src/Utils/ModelLoader.h"
#include "../../src/Utils/TextureLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#define PI 3.14159265359
#define WAVES_COUNT 32

#define QUAD_GRID_VERTEX_COUNT 80
//#define QUAD_GRID_VERTEX_COUNT 15
#define QUAD_VERTEX_DISTANCE 10.0f

class OceanRendering : public Application::IScene {
public:
	OceanRendering() {
		settings.Title = "OceanRendering.exe";
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
		alignas(16) glm::mat4 Projection = glm::mat4(1.0f);
		alignas(16) glm::mat4 View = glm::mat4(1.0f);
		alignas(16) glm::vec4 LightPosition = glm::vec4(80.0f, 20.0f, 10.0f, 0.8f);         // w is light strength
		alignas(16) glm::vec4 LightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.7f);               // w is light specular factor
		alignas(16) glm::vec4 ViewerPosition = glm::vec4(0.0f);
		alignas(16) glm::vec4 WaterColor = glm::vec4(0.005f, 0.02f, 0.05f, 1.0f);           // w is empty
        alignas(4) int Flags = 0;
        alignas(4) int WaveCount = WAVES_COUNT;
        alignas(4) float SpecularDisplacement = 1.0f;
        alignas(4) float WaterShininess = 750.0f;
        alignas(4) float TemporalPhaseExponent = 0.8f;
        alignas(4) float HeightMultiplier = 1.5f;
        alignas(4) float WindAngle = 0.5f;
        alignas(4) float WindSpeed = 2.0f;
        alignas(4) float DragMult = 0.1f;
        alignas(4) float Time = 0.0f;
        alignas(4) float WaterDepth = 8.0f;
//        alignas(4) float SineFBMAmplitude = 1.0f;
        alignas(4) float SineFBMAmplitude = 0.05f;
        alignas(4) float SineFBMFrequency = 0.1f;
        alignas(4) float SineFBMAmplitudeMultiplier = 0.8f;   // must be smaller than 1.0
        alignas(4) float SineFBMFrequencyMultiplier = 1.23;   // must be greater than 1.0
        alignas(4) float TessellationMinThreshold = 300.0f;
        alignas(4) float TessellationMaxThreshold = 200.0f;
        alignas(4) float TessellationLevelMin = 1.0f;
        alignas(4) float TessellationLevelMax = 8.0f;
        alignas(4) float TessellationStep = 5.0f;
        alignas(4) float ReflectionStrength = 0.5f;
    } SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model = glm::mat4(1.0f);
        alignas(16) glm::vec4 Color = glm::vec4(1.0f);
	} FramePushConstants;

	const glm::vec3 InitialCameraPosition = glm::vec3(-397.0f, 7.0f, -15.0f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= -2.3f;
	const float InitialCameraPitch	= -1.9f;
private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

    Graphics::Texture m_SkyboxTexture = {};

    std::shared_ptr<Assets::Model> m_WaterModel;

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_OffscreenRenderTarget;

    Graphics::Shader m_VertexShader = {};
    Graphics::Shader m_TessellationControlShader = {};
    Graphics::Shader m_TessellationEvaluationShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::GPUBuffer m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT] = {};

	Graphics::PipelineState m_DefaultPSO = {};
	Graphics::PipelineState m_WireframePSO = {};

	VkDescriptorSetLayout m_FrameDescriptorSetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_FrameDescriptorSet = { VK_NULL_HANDLE };

    Graphics::Shader m_LightSourceVertexShader = {};
    Graphics::Shader m_LightSourceFragmentShader = {};
    Graphics::PipelineState m_LightSourcePSO = {};

    Graphics::Shader m_SkyboxVertexShader = {};
    Graphics::Shader m_SkyboxFragmentShader = {};
    Graphics::PipelineState m_SkyboxPSO = {};

    glm::vec2 m_AverageWaveDirection = glm::vec2(1.0f, 0.0f);

    float m_SkyboxCubeSize = 1000.0f;

    bool m_RenderWireframe = true;
    bool m_DebugRenderNormals = false;
    bool m_CircularWavesEnabled = false;
    bool m_DebugRenderWorldSpacePos = false;
    bool m_FractalBrownianMotionDomainWarpingEnabled = true;
    bool m_TessellationEnabled = true;
    bool m_ReflectionEnabled = true;
    bool m_RenderSkybox = true;
private:
    void RenderSkybox(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void RenderLightSource(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void RenderCube(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline);
    void Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet);
};

void OceanRendering::RenderSkybox(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("OceanRendering::RenderSkybox");

    FramePushConstants.Color.r = m_SkyboxCubeSize;
    vkCmdPushConstants(commandBuffer, m_SkyboxPSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);
    RenderCube(currentFrame, commandBuffer, &m_SkyboxPSO);
}

void OceanRendering::RenderLightSource(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("OceanRendering::RenderLightSource");

    FramePushConstants.Model = glm::translate(glm::mat4(1.0f), glm::vec3(SampleSceneData.LightPosition));
    vkCmdPushConstants(commandBuffer, m_LightSourcePSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);

    RenderCube(currentFrame, commandBuffer, &m_LightSourcePSO);
}

void OceanRendering::RenderCube(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline) {
	SCOPED_PROFILER_US("OceanRendering::RenderCube");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void OceanRendering::StartUp() {

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

	m_OffscreenRenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);
    m_Camera.Far = 2500.0f;
    m_Camera.MovementSpeed = 0.1f;

    m_WaterModel = ModelLoader::LoadMultiQuadModel(QUAD_GRID_VERTEX_COUNT, QUAD_GRID_VERTEX_COUNT, glm::vec3(0.0f), QUAD_VERTEX_DISTANCE);

	m_WaterModel->ModelIndex = 0;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	Graphics::InputLayout frameInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },			
		}
	};

    m_SkyboxTexture = TextureLoader::LoadCubemapTexture("./Textures/evening_road_01_puresky_4k.hdr");
//    m_SkyboxTexture = TextureLoader::LoadCubemapTexture("./Textures/sunflowers_puresky_4k.hdr");

	gfxDevice->CreateDescriptorSetLayout(m_FrameDescriptorSetLayout, frameInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        m_SceneBuffer[i] = gfxDevice->CreateStorageBuffer(sizeof(SceneData));

        gfxDevice->CreateDescriptorSet(m_FrameDescriptorSetLayout, m_FrameDescriptorSet[i]);
		gfxDevice->WriteDescriptor(frameInputLayout.bindings[0], m_FrameDescriptorSet[i], m_SceneBuffer[i]);
        gfxDevice->WriteDescriptor(frameInputLayout.bindings[1], m_FrameDescriptorSet[i], m_SkyboxTexture);
	}

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/OceanRendering/vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, m_TessellationControlShader, "../src/Samples/OceanRendering/tessellation_control.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_TessellationEvaluationShader, "../src/Samples/OceanRendering/tessellation_evaluation.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/OceanRendering/fragment.glsl");

    Graphics::PipelineStateDescription psoDesc = {};

    psoDesc.Name = "Default PSO";
    psoDesc.vertexShader = &m_VertexShader;
    psoDesc.tessellationControlShader = &m_TessellationControlShader;
    psoDesc.tessellationEvaluationShader = &m_TessellationEvaluationShader;
    psoDesc.tessellationPatchControlPoints = 3;
    psoDesc.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    psoDesc.fragmentShader = &m_FragShader;
    psoDesc.psoInputLayout.push_back(frameInputLayout);
    psoDesc.cullMode = VK_CULL_MODE_NONE;

    gfxDevice->CreatePipelineState(psoDesc, m_DefaultPSO, *m_OffscreenRenderTarget.get());

    psoDesc.Name = "Wireframe PSO";
    psoDesc.lineWidth = 2.0f;
    psoDesc.polygonMode = VK_POLYGON_MODE_LINE;

    gfxDevice->CreatePipelineState(psoDesc, m_WireframePSO, *m_OffscreenRenderTarget.get());

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourceVertexShader, "../src/Samples/OceanRendering/light_source_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourceFragmentShader, "../src/Samples/OceanRendering/light_source_fragment.glsl");

    Graphics::PipelineStateDescription lightSourceDesc = {};
    lightSourceDesc.Name = "Light Source";
    lightSourceDesc.vertexShader = &m_LightSourceVertexShader;
    lightSourceDesc.fragmentShader = &m_LightSourceFragmentShader;
    lightSourceDesc.noVertex = true;
    lightSourceDesc.psoInputLayout.push_back(frameInputLayout);

    gfxDevice->CreatePipelineState(lightSourceDesc, m_LightSourcePSO, *m_OffscreenRenderTarget.get());

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SkyboxVertexShader, "../src/Samples/OceanRendering/skybox_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SkyboxFragmentShader, "../src/Samples/OceanRendering/skybox_fragment.glsl");

    Graphics::PipelineStateDescription skyboxDesc = {};
    skyboxDesc.Name = "Skybox";
    skyboxDesc.vertexShader = &m_SkyboxVertexShader;
    skyboxDesc.fragmentShader = &m_SkyboxFragmentShader;
    skyboxDesc.noVertex = true;
    skyboxDesc.psoInputLayout.push_back(frameInputLayout);

    gfxDevice->CreatePipelineState(skyboxDesc, m_SkyboxPSO, *m_OffscreenRenderTarget.get());
}

void OceanRendering::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();

    for (size_t i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        gfxDevice->DestroyBuffer(m_SceneBuffer[i]);
    }

    gfxDevice->DestroyDescriptorSetLayout(m_FrameDescriptorSetLayout);

    gfxDevice->DestroyShader(m_VertexShader);
    gfxDevice->DestroyShader(m_TessellationControlShader);
    gfxDevice->DestroyShader(m_TessellationEvaluationShader);
   	gfxDevice->DestroyShader(m_FragShader);

	gfxDevice->DestroyPipeline(m_DefaultPSO);
	gfxDevice->DestroyPipeline(m_WireframePSO);

    gfxDevice->DestroyShader(m_LightSourceVertexShader);
    gfxDevice->DestroyShader(m_LightSourceFragmentShader);
	gfxDevice->DestroyPipeline(m_LightSourcePSO);

    gfxDevice->DestroyImage(m_SkyboxTexture);
    gfxDevice->DestroyShader(m_SkyboxVertexShader);
    gfxDevice->DestroyShader(m_SkyboxFragmentShader);
    gfxDevice->DestroyPipeline(m_SkyboxPSO);
}

void OceanRendering::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("OceanRendering::Update");

	m_Camera.OnUpdate(deltaT, input);

    m_WaterModel->OnUpdate(deltaT);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
    SampleSceneData.ViewerPosition  = glm::vec4(m_Camera.Position, 1.0f);
    SampleSceneData.Time            = constantT;
    SampleSceneData.Flags           = (m_ReflectionEnabled << 5
                                        | m_TessellationEnabled << 4
                                        | m_FractalBrownianMotionDomainWarpingEnabled << 3
                                        | m_DebugRenderWorldSpacePos << 2
                                        | m_CircularWavesEnabled << 1
                                        | m_DebugRenderNormals);

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], 0, &SampleSceneData, sizeof(SceneData));
}

void OceanRendering::Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet) {
	SCOPED_PROFILER_US("OceanRendering::Render");

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    gfxDevice->BindDescriptorSet(*frameDescriptorSet, commandBuffer, pipeline->pipelineLayout, 0, 1);

    VkDeviceSize offsets[] = { sizeof(uint32_t) * m_WaterModel->TotalIndices };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_WaterModel->DataBuffer.Handle, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_WaterModel->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

    FramePushConstants.Model = m_WaterModel->GetModelMatrix();

    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);

    for (const auto& mesh: m_WaterModel->Meshes) { 
        vkCmdDrawIndexed(
            commandBuffer, 
            static_cast<uint32_t>(mesh.Indices.size()), 
            1, 
            static_cast<uint32_t>(mesh.IndexOffset), 
            static_cast<int32_t>(mesh.VertexOffset),
            0);
    }
}

void OceanRendering::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("OceanRendering::RenderScene");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget->Begin(commandBuffer);

    if (m_RenderWireframe) {
        Render(currentFrame, commandBuffer, &m_WireframePSO, &m_FrameDescriptorSet[currentFrame]);
    } else {
        Render(currentFrame, commandBuffer, &m_DefaultPSO, &m_FrameDescriptorSet[currentFrame]);
    }

    RenderLightSource(currentFrame, commandBuffer);

    if (m_RenderSkybox) {
        RenderSkybox(currentFrame, commandBuffer);
    }

	m_OffscreenRenderTarget->End(commandBuffer);

	m_OffscreenRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_OffscreenRenderTarget->GetColorBuffer());
}

void OceanRendering::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

    if (ImGui::TreeNode("Skybox Settings")) {
        ImGui::DragFloat("Cube Size", &m_SkyboxCubeSize, 1.0f, 0.0f, 2000.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Pipeline Settings")) {
        ImGui::Checkbox("Render Wireframe",				    &m_RenderWireframe);
        ImGui::Checkbox("Render Skybox",                    &m_RenderSkybox);
        ImGui::Checkbox("Tessellation Enabled",             &m_TessellationEnabled);
        ImGui::Checkbox("Reflection Enabled",               &m_ReflectionEnabled);
        ImGui::Checkbox("Debug - Render World Space Pos",   &m_DebugRenderWorldSpacePos);
        ImGui::Checkbox("Debug - Render Normals",           &m_DebugRenderNormals);

        if (m_TessellationEnabled) {
            ImGui::DragFloat("Tessellation Min Threshold",  &SampleSceneData.TessellationMinThreshold, 1.0f, 0.0f, 500.0f);
            ImGui::DragFloat("Tessellation Max Threshold",  &SampleSceneData.TessellationMaxThreshold, 1.0f, 0.0f, 500.0f);
            ImGui::DragFloat("Tessellation Level Min",      &SampleSceneData.TessellationLevelMin, 1.0f, 1.0f, 64.0f);
            ImGui::DragFloat("Tessellation Level Max",      &SampleSceneData.TessellationLevelMax, 1.0f, 1.0f, 64.0f);
            ImGui::DragFloat("Tessellation Step",           &SampleSceneData.TessellationStep, 0.01f, 0.0f, 5.0f);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Water Material Settings")) {
        ImGui::ColorPicker3("Deep Water Color",         (float*)&SampleSceneData.WaterColor);
        ImGui::DragFloat("Water Shininess",             &SampleSceneData.WaterShininess, 1.0f, 0.0f, 3000.0f);
        ImGui::DragFloat("Water Reflection Strength" ,  &SampleSceneData.ReflectionStrength, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat("Specular Displacement",       &SampleSceneData.SpecularDisplacement, 0.01f, -10.0f, 10.0f);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Light Settings")) {
        ImGui::DragFloat3("Light Position",	(float*)&SampleSceneData.LightPosition, 0.2f, -1000.0f, 1000.0f);
        ImGui::DragFloat("Light Strength",  &SampleSceneData.LightPosition.w, 0.02f, 0.0f, 200.0f);
        ImGui::ColorPicker3("Light Color",  (float*)&SampleSceneData.LightColor);
        ImGui::DragFloat("Light Specular",	&SampleSceneData.LightColor.w, 0.02f, 0.0f, 1.0f);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Wave Settings")) {
        ImGui::Checkbox("Circular Waves Enabled",                           &m_CircularWavesEnabled);
        ImGui::Checkbox("Fractal Brownian Motion Domain Warping Enabled",   &m_FractalBrownianMotionDomainWarpingEnabled);
        ImGui::DragFloat("Water Depth",                                     &SampleSceneData.WaterDepth, 0.01f, 0.0f, 50.0f);
        ImGui::DragFloat("Sine FBM Wave Amplitude",                         &SampleSceneData.SineFBMAmplitude, 0.001f, 0.0f, 100.0f);
        ImGui::DragFloat("Sine FBM Wave Frequency",                         &SampleSceneData.SineFBMFrequency, 0.001f, 0.0f, 100.0f);
        ImGui::DragFloat("Sine FBM Wave Amplitude Multiplier",              &SampleSceneData.SineFBMAmplitudeMultiplier, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Sine FBM Wave Frequency Multiplier",              &SampleSceneData.SineFBMFrequencyMultiplier, 0.001f, 1.0f, 10.0f);
        ImGui::DragFloat("Drag Mult",                                       &SampleSceneData.DragMult, 0.001f, 0.0f, 10.0f);
        ImGui::DragFloat("Wind Angle",                                      &SampleSceneData.WindAngle, 0.01f, -90.0f, 90.0f);
        ImGui::DragFloat("Wind Speed",                                      &SampleSceneData.WindSpeed, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Temporal Phase Exponent",                         &SampleSceneData.TemporalPhaseExponent, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Height Multiplier",                               &SampleSceneData.HeightMultiplier, 0.01f, 0.0f, 10.0f);

        ImGui::TreePop();
    }

	ImGui::SeparatorText("Model Settings");
    m_WaterModel->OnUIRender();
}

void OceanRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

RUN_APPLICATION(OceanRendering);
