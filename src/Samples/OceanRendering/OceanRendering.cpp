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

#define PI 3.14159265359
#define SINE_WAVES_MAX 3

/*
    My pipeline expectation for this project:
        - Draw call of a simple quad formed by 2 triangles
        - Simple vertex shader where we don't even multiply by the MVP matrices
        - Tesselation stage where the quad is split into thousands of triangles and MVP matrices multiplication happens
        - Fragment shader for lighing and color
        - Only noise texture will be used (if used)

    
    Topics to look into and implement:
        - Simpler wave simulation: Gerstner waves (or sine/trochoidal wave)
        - More realistic approach: FFT (Fast Fourier Transform)
        - Normals and micro-detail computation
            - Derive surface normals from wave slopes (either analysitcally from the wave function or from a normal map generated alongside displacement).
        - Water material
            - Fresnel term for view-dependent reflection/refraction.
            - Reflection: Sample from a dynamic reflection probe, cubemap, or rendered reflection texture (planar or screen space)
            - Refraction: Sample scene color with distortion based on normals/wave height (possibly with a separate refraction render target)
            - Color/absorption: Depth-based tinting (deeper water darker/bluer) and subsurface scattering approximation
            - Specular/highlights: Strong sun reflections and environmental specular
            - Foam and details: Add foam on wave crests (based on height/slope)
        - Supporting effects
            - Caustics: Project or render light patterns underwater
            - Interactions: Basic buoyancy for objects, shoreline blending, or particle systems for breaking waves/spray.
            - Environmental integration: match lighting to the sky, add wind direction influence on waves.
        - Optimizations:
            - LOD, Frustum culling, and wave culling for distant areas.
            - Render at lower resolution or with lower simulation resolution far away.
            - Profile compute shaders (FFT can be heavy) and consider multi-threading or pre-computation where possible.
            - Handle horizon clipping and large-world positioning (e.g., grid snapping to camera).
*/

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

    // Note: X and Z are directions, Y is length and W is speed.
    struct WaveData {
        alignas(16) glm::vec4 Direction = glm::vec4(0.0f);
        alignas(4) float Amplitude = 0.0f;
    } WaveGPUData[SINE_WAVES_MAX];

    struct WaveDataCPU {
        glm::vec2 Direction = glm::vec2(0.0f);
        float Length = 0.0;
        float Speed = 0.0f;
        float Amplitude = 0.0f;
    } WaveCPUData[SINE_WAVES_MAX];

	struct SceneData {
		alignas(16) glm::mat4 Projection;
		alignas(16) glm::mat4 View;
        alignas(16) glm::vec4 Padding[3];
		alignas(16) glm::vec4 LightPosition;
		alignas(16) glm::vec4 ViewerPosition;
        // Note: W is specular factor
		alignas(16) glm::vec4 WaterColor = glm::vec4(1.0f);
        alignas(4) int Flags;
        alignas(4) int SineWaveCount = 1;
        alignas(4) float TessellationLevelInner = 64.0f;
        alignas(4) float TessellationLevelOuter = 64.0f;
        alignas(4) float Time = 0.0f;
        alignas(4) float DeltaT = 0.0f;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model = glm::mat4(1.0f);
        alignas(16) glm::vec4 Color = glm::vec4(1.0f);
	} FramePushConstants;

	const glm::vec3 InitialCameraPosition = glm::vec3(-15.0f, 12.0f, 17.0f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= -52.0f;
	const float InitialCameraPitch	= -33.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

	std::shared_ptr<Assets::Model> m_OceanModel;

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_OffscreenRenderTarget;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_TessellationControlShader = {};
	Graphics::Shader m_TessellationEvaluationShader = {};
	Graphics::Shader m_FragShader = {};
    Graphics::Shader m_VertexShaderTessellationDisabled = {};

	Graphics::Buffer m_SceneBuffer = {};
    Graphics::Buffer m_SineWavesBuffer = {};

	Graphics::PipelineState m_DefaultPSO = {};
	Graphics::PipelineState m_WireframePSO = {};
	Graphics::PipelineState m_DefaultTessellationDisabledPSO = {};
    Graphics::PipelineState m_WireframeTessellationDisabledPSO = {};

	VkDescriptorSetLayout m_FrameDescriptorSetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_FrameDescriptorSet = { VK_NULL_HANDLE };

    VkDescriptorSetLayout m_FrameTessellationDisabledDescriptorSetLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_FrameTessellationDisabledDescriptorSet = { VK_NULL_HANDLE };

    Graphics::Shader m_LightSourceVertexShader = {};
    Graphics::Shader m_LightSourceFragmentShader = {};

    Graphics::PipelineState m_LightSourcePSO = {};
    Graphics::PipelineState m_LightSourceTessellationDisabledPSO = {};

    VkDescriptorSetLayout m_LightSourceDescriptorSetLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_LightSourceDescriptorSet = { VK_NULL_HANDLE };

	glm::vec4 m_LightPosition = glm::vec4(1.0f, 8.0f, 1.0f, 1.0f);
    glm::vec3 m_WaterColor = glm::vec3(0.09f, 0.55f, 0.79f);

    float m_OrbitalLightSpeed = 0.5f;
	float m_OrbitalLightDisplacement = 3.0f;
    float m_WaterSpecularFactor = 32.0f; 

    bool m_TessellationEnabled = true;
    bool m_OrbitateLight = false;
    bool m_RenderWireframe = false;
    bool m_SineWave = false;
    bool m_DebugRenderNormals = false;
    bool m_GenerateNormalPerFragment = false;

private:
    void RenderLightSource(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline);
    void Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet, bool drawIndexed = false);
};

void OceanRendering::RenderLightSource(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline) {
	SCOPED_PROFILER_US("OceanRendering::RenderLightSource");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    FramePushConstants.Model = glm::translate(glm::mat4(1.0f), glm::vec3(m_LightPosition));

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdPushConstants(commandBuffer, m_LightSourcePSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void OceanRendering::StartUp() {

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

	m_OffscreenRenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

	m_OceanModel = ModelLoader::LoadModel(ModelType::QUAD);
	m_OceanModel->Transformations.translation.y = -0.51f;
//	m_OceanModel->Transformations.rotation.x = 90.0f;
	m_OceanModel->Transformations.scaleHandler = 20.0f;
	m_OceanModel->ModelIndex = 0;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/OceanRendering/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, m_TessellationControlShader, "../src/Samples/OceanRendering/tessellation_control.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_TessellationEvaluationShader, "../src/Samples/OceanRendering/tessellation_evaluation.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/OceanRendering/fragment.glsl");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(SceneData));
	m_SineWavesBuffer = gfxDevice->CreateBuffer(sizeof(WaveData) * SINE_WAVES_MAX);

    WaveCPUData[0].Length = 0.6f;
    WaveCPUData[0].Amplitude = 0.025f;
    WaveCPUData[0].Speed = 0.1f;
    WaveCPUData[0].Direction = glm::vec2(1.0f, 1.0f);

    WaveCPUData[1].Length = 0.310f;
    WaveCPUData[1].Amplitude = 0.025f;
    WaveCPUData[1].Speed = 0.1f;
    WaveCPUData[1].Direction = glm::vec2(1.0f, 0.6f);

    WaveCPUData[2].Length = 0.180f;
    WaveCPUData[2].Amplitude = 0.025f;
    WaveCPUData[2].Speed = 0.1f;
    WaveCPUData[2].Direction = glm::vec2(1.0f, 1.3f);

    SampleSceneData.SineWaveCount = 3;

	Graphics::InputLayout frameInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
		}
	};

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Default - Tessellation enabled";
	desc.vertexShader = &m_VertexShader;
	desc.tessellationControlShader = &m_TessellationControlShader;
	desc.tessellationEvaluationShader = &m_TessellationEvaluationShader;
    desc.tessellationPatchControlPoints = 4;
    desc.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	desc.fragmentShader = &m_FragShader;
	desc.psoInputLayout.push_back(frameInputLayout);
    desc.cullMode = VK_CULL_MODE_NONE;      // TEMPORARY for testing purposes

    gfxDevice->CreatePipelineState(desc, m_DefaultPSO, *m_OffscreenRenderTarget.get());

	desc.Name = "Wireframe";
    desc.lineWidth = 2.0f;
    desc.polygonMode = VK_POLYGON_MODE_LINE;

    gfxDevice->CreatePipelineState(desc, m_WireframePSO, *m_OffscreenRenderTarget.get());

	gfxDevice->CreateDescriptorSetLayout(m_FrameDescriptorSetLayout, frameInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_FrameDescriptorSetLayout, m_FrameDescriptorSet[i]);
		gfxDevice->WriteDescriptor(frameInputLayout.bindings[0], m_FrameDescriptorSet[i], m_SceneBuffer);
		gfxDevice->WriteDescriptor(frameInputLayout.bindings[1], m_FrameDescriptorSet[i], m_SineWavesBuffer);
	}

    Graphics::InputLayout frameTessellationDisabledInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
		}
    };

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShaderTessellationDisabled, "../src/Samples/OceanRendering/vertex_tessellation_disabled.glsl");

    Graphics::PipelineStateDescription tessellationDisabledPSODesc = {};

    tessellationDisabledPSODesc.Name = "Default - Tessellation disabled";
    tessellationDisabledPSODesc.vertexShader = &m_VertexShaderTessellationDisabled;
    tessellationDisabledPSODesc.fragmentShader = &m_FragShader;
    tessellationDisabledPSODesc.psoInputLayout.push_back(frameTessellationDisabledInputLayout);

    gfxDevice->CreatePipelineState(tessellationDisabledPSODesc, m_DefaultTessellationDisabledPSO, *m_OffscreenRenderTarget.get());

    tessellationDisabledPSODesc.Name = "Wireframe - Tessellation disabled";
    tessellationDisabledPSODesc.lineWidth = 2.0f;
    tessellationDisabledPSODesc.polygonMode = VK_POLYGON_MODE_LINE;

    gfxDevice->CreatePipelineState(tessellationDisabledPSODesc, m_WireframeTessellationDisabledPSO, *m_OffscreenRenderTarget.get());

    gfxDevice->CreateDescriptorSetLayout(m_FrameTessellationDisabledDescriptorSetLayout, frameTessellationDisabledInputLayout.bindings);

    for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        gfxDevice->CreateDescriptorSet(m_FrameTessellationDisabledDescriptorSetLayout, m_FrameTessellationDisabledDescriptorSet[i]);
        gfxDevice->WriteDescriptor(frameTessellationDisabledInputLayout.bindings[0], m_FrameTessellationDisabledDescriptorSet[i], m_SceneBuffer);
        gfxDevice->WriteDescriptor(frameTessellationDisabledInputLayout.bindings[1], m_FrameTessellationDisabledDescriptorSet[i], m_SineWavesBuffer);
    }

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourceVertexShader, "../src/Samples/OceanRendering/light_source_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourceFragmentShader, "../src/Samples/OceanRendering/light_source_fragment.glsl");

    Graphics::PipelineStateDescription lightSourceDesc = {};
    lightSourceDesc.Name = "Light Source";
    lightSourceDesc.vertexShader = &m_LightSourceVertexShader;
    lightSourceDesc.fragmentShader = &m_LightSourceFragmentShader;
    lightSourceDesc.noVertex = true;
    lightSourceDesc.psoInputLayout.push_back(frameInputLayout);

    gfxDevice->CreatePipelineState(lightSourceDesc, m_LightSourcePSO, *m_OffscreenRenderTarget.get());

    Graphics::PipelineStateDescription lightSourceTessellationDisabledDesc = {};
    lightSourceTessellationDisabledDesc.Name = "Light Source - Tessellation disabled";
    lightSourceTessellationDisabledDesc.vertexShader = &m_LightSourceVertexShader;
    lightSourceTessellationDisabledDesc.fragmentShader = &m_LightSourceFragmentShader;
    lightSourceTessellationDisabledDesc.noVertex = true;
    lightSourceTessellationDisabledDesc.psoInputLayout.push_back(frameTessellationDisabledInputLayout);

    gfxDevice->CreatePipelineState(lightSourceTessellationDisabledDesc, m_LightSourceTessellationDisabledPSO, *m_OffscreenRenderTarget.get());
}

void OceanRendering::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_TessellationControlShader);
	gfxDevice->DestroyShader(m_TessellationEvaluationShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_FrameDescriptorSetLayout);
	gfxDevice->DestroyPipeline(m_DefaultPSO);
	gfxDevice->DestroyPipeline(m_WireframePSO);
	gfxDevice->DestroyPipeline(m_DefaultTessellationDisabledPSO);
	gfxDevice->DestroyPipeline(m_WireframeTessellationDisabledPSO);
	gfxDevice->DestroyDescriptorSetLayout(m_FrameTessellationDisabledDescriptorSetLayout);
    gfxDevice->DestroyShader(m_VertexShaderTessellationDisabled);

    gfxDevice->DestroyShader(m_LightSourceVertexShader);
    gfxDevice->DestroyShader(m_LightSourceFragmentShader);
	gfxDevice->DestroyPipeline(m_LightSourcePSO);
	gfxDevice->DestroyPipeline(m_LightSourceTessellationDisabledPSO);
}

void OceanRendering::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("OceanRendering::Update");

	m_Camera.OnUpdate(deltaT, input);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	if (m_OrbitateLight) {
		m_LightPosition.x = glm::sin(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
		m_LightPosition.z = glm::cos(constantT + m_OrbitalLightSpeed) * m_OrbitalLightDisplacement;
	}

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
	SampleSceneData.LightPosition	= m_LightPosition;
    SampleSceneData.ViewerPosition  = glm::vec4(m_Camera.Position, 1.0f);
    SampleSceneData.Time            = constantT;
    SampleSceneData.DeltaT          = deltaT;
    SampleSceneData.WaterColor      = glm::vec4(m_WaterColor, m_WaterSpecularFactor);
    SampleSceneData.Flags           = ((m_GenerateNormalPerFragment << 2) | (m_DebugRenderNormals << 1) | m_SineWave);

    for (int WaveIndex = 0; WaveIndex < SampleSceneData.SineWaveCount; WaveIndex++) {

        WaveData *WaveGPU = &WaveGPUData[WaveIndex];
        WaveDataCPU *WaveCPU = &WaveCPUData[WaveIndex];

        WaveGPU->Direction.x = WaveCPU->Direction.x;
        WaveGPU->Direction.z = WaveCPU->Direction.y;
        WaveGPU->Direction.y = (2 * PI) / WaveCPU->Length;
        WaveGPU->Direction.w = WaveCPU->Speed * WaveGPU->Direction.y;
        WaveGPU->Amplitude = WaveCPU->Amplitude;
    }

	gfxDevice->UpdateBuffer(m_SceneBuffer, &SampleSceneData);
	gfxDevice->UpdateBuffer(m_SineWavesBuffer, &WaveGPUData);
}

void OceanRendering::Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet, bool drawIndexed) {
	SCOPED_PROFILER_US("OceanRendering::Render");

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    gfxDevice->BindDescriptorSet(*frameDescriptorSet, commandBuffer, pipeline->pipelineLayout, 0, 1);

    Assets::Model& Model = *m_OceanModel.get();

    VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
    vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

    FramePushConstants.Model = Model.GetModelMatrix();

    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);

    for (const auto& Mesh: Model.Meshes) { 
        if (drawIndexed) {
            vkCmdDrawIndexed(
                commandBuffer, 
                static_cast<uint32_t>(Mesh.Indices.size()), 
                1, 
                static_cast<uint32_t>(Mesh.IndexOffset), 
                static_cast<int32_t>(Mesh.VertexOffset),
                0);
        } else {
            // Note: Must use vkCmdDraw instead of vkCmdDrawIndexed to tessellate quads.
            vkCmdDraw(commandBuffer, Mesh.Vertices.size(), Mesh.Indices.size(), 0, 0);
        }
    }
}

void OceanRendering::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("OceanRendering::RenderScene");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget->Begin(commandBuffer);

    if (m_TessellationEnabled) {
        if (m_RenderWireframe) {
            Render(currentFrame, commandBuffer, &m_WireframePSO, &m_FrameDescriptorSet[currentFrame]);
        } else {
            Render(currentFrame, commandBuffer, &m_DefaultPSO, &m_FrameDescriptorSet[currentFrame]);
        }

        RenderLightSource(currentFrame, commandBuffer, &m_LightSourcePSO);
    } else {
        if (m_RenderWireframe) {
            Render(currentFrame, commandBuffer, &m_WireframeTessellationDisabledPSO, &m_FrameTessellationDisabledDescriptorSet[currentFrame], true);
        } else {
            Render(currentFrame, commandBuffer, &m_DefaultTessellationDisabledPSO, &m_FrameTessellationDisabledDescriptorSet[currentFrame], true);
        }

        RenderLightSource(currentFrame, commandBuffer, &m_LightSourceTessellationDisabledPSO);
    }


	m_OffscreenRenderTarget->End(commandBuffer);

	m_OffscreenRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_OffscreenRenderTarget->GetColorBuffer());
}

void OceanRendering::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

    ImGui::ColorPicker3("Water Color",              (float*)&m_WaterColor);
    ImGui::DragFloat("Water Specular factor",       &m_WaterSpecularFactor, 2.0f, 0.0f, 64.0f);
    ImGui::Checkbox("Render Wireframe",				&m_RenderWireframe);
    ImGui::Checkbox("Tessellation Enabled",         &m_TessellationEnabled);
    // Note: DragFloat signature -> const char *label, float *value, float speed, float min, float max
    // Note: Vulkan tessellation levels through dedicated tessellation pipeline stop at 64. To achieve higher levels we should we use compute shaders instead.
    ImGui::DragFloat("Tessellation Level Inner",    &SampleSceneData.TessellationLevelInner, 1.0f, 1.0f, 64.0f);
    ImGui::DragFloat("Tessellation Level Outer",    &SampleSceneData.TessellationLevelOuter, 1.0f, 1.0f, 64.0f);
    ImGui::Checkbox("Sine Wave",                    &m_SineWave);
    ImGui::Checkbox("Generate Normal per fragment", &m_GenerateNormalPerFragment);
    ImGui::Checkbox("Debug - Render Normals",       &m_DebugRenderNormals);
	ImGui::Checkbox("Orbitate Light",				&m_OrbitateLight);
	ImGui::DragFloat("Light Orbital Speed",			&m_OrbitalLightSpeed, 0.02f, 0.0f, 3.0f);
	ImGui::DragFloat("Light Orbital Displacement",	&m_OrbitalLightDisplacement, 0.02f, 0.0f, 9.0f);
	ImGui::DragFloat4("Light Position",				(float*)&m_LightPosition, 0.2f, -20.0f, 20.0f);

    ImGui::DragInt("Active Sine Waves", &SampleSceneData.SineWaveCount, 1, 1, static_cast<int>(SINE_WAVES_MAX));

    if (ImGui::TreeNode("Sine Waves Settings")) {
        for (int WaveIndex = 0; WaveIndex < SINE_WAVES_MAX; WaveIndex++) {
            std::string WaveId = "wave_" + std::to_string(WaveIndex);

            if (ImGui::TreeNode(WaveId.c_str())) {
                ImGui::DragFloat("Wave Length",                 &WaveCPUData[WaveIndex].Length, 0.01f, 0.0f, 100.0f);
                ImGui::DragFloat("Wave Amplitude",              &WaveCPUData[WaveIndex].Amplitude, 0.001f, 0.0f, 10.00f);
                ImGui::DragFloat("Wave Speed",                  &WaveCPUData[WaveIndex].Speed, 0.001f, 0.0f, 10.00f);
                ImGui::DragFloat2("Wave Direction (X and Z)",   (float*)&WaveCPUData[WaveIndex].Direction, 0.001f, -1.0f, 1.0f);

                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    m_OceanModel->OnUIRender();
}

void OceanRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

RUN_APPLICATION(OceanRendering);
