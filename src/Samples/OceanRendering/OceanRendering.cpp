#include <iostream>
#include <random>
#include <vector>

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
#define SINE_WAVES_MAX 32

/*
 
    Notes:
        -   Tessellation might not be the best approach due to the limitation 
            of 64 inner/outer levels. I will first fix normal generation and 
            then change the approach from 4 vertices tessellation to thousands
            of vertices without tessellation.

        
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
        alignas(16) glm::vec4 CircularWave = glm::vec4(0.0f);
        alignas(4) float Amplitude = 0.0f;
        alignas(4) float Steepness = 0.0f;
    } WaveGPUData[SINE_WAVES_MAX];

    struct WaveDataCPU {
        glm::vec2 Direction = glm::vec2(0.0f);
        glm::vec2 CircularWaveCenter = glm::vec2(0.0f);
        float Length = 0.0;
        float Speed = 0.0f;
        float Amplitude = 0.0f;
        float Steepness = 0.0f;
    } WaveCPUData[SINE_WAVES_MAX];

    // TODO: add tessellation 
    struct SceneData {
		alignas(16) glm::mat4 Projection = glm::mat4(1.0f);
		alignas(16) glm::mat4 View = glm::mat4(1.0f);
		alignas(16) glm::vec4 LightPosition = glm::vec4(80.0f, 20.0f, 10.0f, 0.8f); // w is light strength
		alignas(16) glm::vec4 LightColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.7f);       // w is light specular factor
		alignas(16) glm::vec4 ViewerPosition = glm::vec4(0.0f);
        alignas(4) int Flags = 0;
        alignas(4) int WaveCount = 1;
        alignas(4) float SpecularDisplacement = 1.0f;
        alignas(4) float WaterShininess = 750.0f;
        alignas(4) float TemporalPhaseExponent = 0.8f;
        alignas(4) float HeightMultiplier = 1.5f;
        alignas(4) float WindAngle = 0.5f;
        alignas(4) float WindSpeed = 2.0f;
        alignas(4) float DragMult = 0.1f;
        alignas(4) float Time = 0.0f;
        alignas(4) float WaterDepth = 5.0f;
        alignas(4) float SineFBMAmplitude = 0.4f;
        alignas(4) float SineFBMFrequency = 0.1f;
        alignas(4) float SineFBMAmplitudeMultiplier = 0.8f;   // must be smaller than 1.0
        alignas(4) float SineFBMFrequencyMultiplier = 1.2;    // must be greater than 1.0
        alignas(4) float Padding1 = 0.0f;
        alignas(4) float Padding2 = 0.0f;
        alignas(4) float Padding3 = 0.0f;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model = glm::mat4(1.0f);
        alignas(16) glm::vec4 Color = glm::vec4(1.0f);
	} FramePushConstants;

//	const glm::vec3 InitialCameraPosition = glm::vec3(-15.0f, 12.0f, 17.0f);
	const glm::vec3 InitialCameraPosition = glm::vec3(-83.0f, 80.0f, 162.0f);

	const float InitialCameraFov	= 45.0f;

//    const float InitialCameraYaw	= -52.0f;
//	const float InitialCameraPitch	= -33.0f;

	const float InitialCameraYaw	= -64.0f;
	const float InitialCameraPitch	= -23.0f;
private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

    std::shared_ptr<Assets::Model> m_WaterModel;

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_OffscreenRenderTarget;

    Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::Buffer m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT] = {};
    Graphics::Buffer m_WavesBuffer = {};

	Graphics::PipelineState m_DefaultPSO = {};
	Graphics::PipelineState m_WireframePSO = {};

	VkDescriptorSetLayout m_FrameDescriptorSetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_FrameDescriptorSet = { VK_NULL_HANDLE };

    Graphics::Shader m_LightSourceVertexShader = {};
    Graphics::Shader m_LightSourceFragmentShader = {};

    Graphics::PipelineState m_LightSourcePSO = {};

    VkDescriptorSetLayout m_LightSourceDescriptorSetLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_LightSourceDescriptorSet = { VK_NULL_HANDLE };

    glm::vec2 m_AverageWaveDirection = glm::vec2(1.0f, 0.0f);

    float m_AverageWaveLength = 7.0f;
    float m_AverageWaveAmplitude = 0.110f;
    float m_GravitationalConstant = 9.8f; // m/s^2
    float m_DirectionDeviation = 0.170f; 
    float m_AverageWaveSteepness = 11.0f;
    float m_WaveSteepnessDeviation = 2.4f;

    bool m_RenderWireframe = false;
    bool m_SineWave = true;
    bool m_GerstnerWave = false;
    bool m_DebugRenderNormals = false;
    bool m_GenerateRandomWaveData = false;
    bool m_CircularWavesEnabled = false;
    bool m_DebugRenderWorldSpacePos = false;
    bool m_SineWaveFractalBrownianMotion = false;
    bool m_FractalBrownianMotionDomainWarpingEnabled = false;

private:
    void RenderLightSource(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline);
    void Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet);

    void PrintActiveWaveCPUData();
    void GenerateWaveData();
};

void OceanRendering::PrintActiveWaveCPUData() {

    for (size_t waveIndex = 0; waveIndex < SampleSceneData.WaveCount; ++waveIndex) {

        if (waveIndex == 0) {
            std::cout << "--- Big base waves ---\n";
        } else if (waveIndex == 6) {
            std::cout << "--- Medium waves ---\n";
        } else if (waveIndex == 22) {
            std::cout << "--- fast surface waves ---\n";
        }

        const WaveDataCPU &wave = WaveCPUData[waveIndex];

        std::cout << "WaveCPUData[" << waveIndex << "].Direction.x = " << wave.Direction.x << "f; ";
        std::cout << "WaveCPUData[" << waveIndex << "].Direction.y = " << wave.Direction.y << "f; ";
        std::cout << "WaveCPUData[" << waveIndex << "].Length  = " << wave.Length << "f; ";
        std::cout << "WaveCPUData[" << waveIndex << "].Speed = " << wave.Speed << "f; ";
        std::cout << "WaveCPUData[" << waveIndex << "].Amplitude = " << wave.Amplitude << "f; ";
        std::cout << "WaveCPUData[" << waveIndex << "].Steepness = " << wave.Steepness << "f; ";
        
        std::cout << '\n';
    }
}

void OceanRendering::GenerateWaveData() {

    const float halfAverageWaveLength = m_AverageWaveLength * 0.5f;
    const float doubleAverageWaveLength = m_AverageWaveLength * 2.0f;

    const float halfAverageWaveAmplitude = m_AverageWaveAmplitude * 0.5f;
    const float doubleAverageWaveAmplitude = m_AverageWaveAmplitude * 2.0f;

    const float minDirectionX = -1.0f * (m_AverageWaveDirection.x + m_DirectionDeviation);
    const float maxDirectionX = m_AverageWaveDirection.x + m_DirectionDeviation;

    const float minDirectionY = -1.0f * (m_AverageWaveDirection.y + m_DirectionDeviation);
    const float maxDirectionY = m_AverageWaveDirection.y + m_DirectionDeviation;

    std::uniform_real_distribution<float> randomWaveLength(halfAverageWaveLength, doubleAverageWaveLength);
    std::uniform_real_distribution<float> randomWaveAmplitude(halfAverageWaveAmplitude, doubleAverageWaveAmplitude);
    std::uniform_real_distribution<float> randomWaveDirectionX(minDirectionX, maxDirectionX);
    std::uniform_real_distribution<float> randomWaveDirectionY(minDirectionY, maxDirectionY);

    std::default_random_engine generator;

    for (size_t waveIndex = 0; waveIndex < SINE_WAVES_MAX; ++waveIndex) {
        WaveCPUData[waveIndex].Length = randomWaveLength(generator);
        WaveCPUData[waveIndex].Amplitude = randomWaveAmplitude(generator);
        WaveCPUData[waveIndex].Speed = sqrt(m_GravitationalConstant * ((2 * PI) / WaveCPUData[waveIndex].Length));
        WaveCPUData[waveIndex].Direction.x = randomWaveDirectionX(generator);
        WaveCPUData[waveIndex].Direction.y = randomWaveDirectionY(generator);

        const float minWaveSteepness = (m_AverageWaveSteepness / WaveCPUData[waveIndex].Length) - m_WaveSteepnessDeviation;
        const float maxWaveSteepness = (m_AverageWaveSteepness / WaveCPUData[waveIndex].Length) + m_WaveSteepnessDeviation;

        std::uniform_real_distribution<float> randomWaveSteepness(minWaveSteepness, maxWaveSteepness);
         
        WaveCPUData[waveIndex].Steepness = std::min(std::max(randomWaveSteepness(generator), 1.0f), 10.0f);
    }
}

void OceanRendering::RenderLightSource(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline) {
	SCOPED_PROFILER_US("OceanRendering::RenderLightSource");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    FramePushConstants.Model = glm::translate(glm::mat4(1.0f), glm::vec3(SampleSceneData.LightPosition));

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdPushConstants(commandBuffer, m_LightSourcePSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void OceanRendering::StartUp() {

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

	m_OffscreenRenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);
    m_Camera.Far = 2000.0f;
    m_Camera.MovementSpeed = 0.1f;

//    m_WaterModel = ModelLoader::LoadMultiQuadModel(256, 256, glm::vec3(0.0f), 0.25f);
//    m_WaterModel = ModelLoader::LoadMultiQuadModel(512, 512, glm::vec3(0.0f), 0.25f);
    m_WaterModel = ModelLoader::LoadMultiQuadModel(1024, 1024, glm::vec3(0.0f), 0.1f);

	m_WaterModel->ModelIndex = 1;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    GenerateWaveData();

    SampleSceneData.WaveCount = 1;

	Graphics::InputLayout frameInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
		}
	};

    /*
        Note: tessellation options to include in the PSO creation.

        Graphics::InputLayout frameInputLayout = {
            .pushConstants = {
                { VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
            },
            .bindings = {
                { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
                { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
            }
        };



        desc.tessellationControlShader = &m_TessellationControlShader;
        desc.tessellationEvaluationShader = &m_TessellationEvaluationShader;
        desc.tessellationPatchControlPoints = 4;
        desc.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    */

	m_WavesBuffer = gfxDevice->CreateBuffer(sizeof(WaveData) * SINE_WAVES_MAX);

	gfxDevice->CreateDescriptorSetLayout(m_FrameDescriptorSetLayout, frameInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        m_SceneBuffer[i] = gfxDevice->CreateBuffer(sizeof(SceneData));

        gfxDevice->CreateDescriptorSet(m_FrameDescriptorSetLayout, m_FrameDescriptorSet[i]);
		gfxDevice->WriteDescriptor(frameInputLayout.bindings[0], m_FrameDescriptorSet[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(frameInputLayout.bindings[1], m_FrameDescriptorSet[i], m_WavesBuffer);
	}

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/OceanRendering/vertex_tessellation_disabled.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/OceanRendering/fragment.glsl");

    Graphics::PipelineStateDescription psoDesc = {};

    psoDesc.Name = "Default PSO";
    psoDesc.vertexShader = &m_VertexShader;
    psoDesc.fragmentShader = &m_FragShader;
    psoDesc.psoInputLayout.push_back(frameInputLayout);

    gfxDevice->CreatePipelineState(psoDesc, m_DefaultPSO, *m_OffscreenRenderTarget.get());

    psoDesc.Name = "Wireframe PSO";
    psoDesc.lineWidth = 2.0f;
    psoDesc.polygonMode = VK_POLYGON_MODE_LINE;

    gfxDevice->CreatePipelineState(psoDesc, m_WireframePSO, *m_OffscreenRenderTarget.get());

    gfxDevice->CreateDescriptorSetLayout(m_LightSourceDescriptorSetLayout, frameInputLayout.bindings);

    for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        gfxDevice->CreateDescriptorSet(m_FrameDescriptorSetLayout, m_FrameDescriptorSet[i]);
        gfxDevice->WriteDescriptor(frameInputLayout.bindings[0], m_FrameDescriptorSet[i], m_SceneBuffer[i]);
        gfxDevice->WriteDescriptor(frameInputLayout.bindings[1], m_FrameDescriptorSet[i], m_WavesBuffer);
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
}

void OceanRendering::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();

    gfxDevice->DestroyDescriptorSetLayout(m_FrameDescriptorSetLayout);
	gfxDevice->DestroyDescriptorSetLayout(m_LightSourceDescriptorSetLayout);

    gfxDevice->DestroyShader(m_VertexShader);
   	gfxDevice->DestroyShader(m_FragShader);

	gfxDevice->DestroyPipeline(m_DefaultPSO);
	gfxDevice->DestroyPipeline(m_WireframePSO);

    gfxDevice->DestroyShader(m_LightSourceVertexShader);
    gfxDevice->DestroyShader(m_LightSourceFragmentShader);
	gfxDevice->DestroyPipeline(m_LightSourcePSO);
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
    SampleSceneData.Flags           = (m_FractalBrownianMotionDomainWarpingEnabled << 6
                                        | m_SineWaveFractalBrownianMotion << 5 
                                        | m_DebugRenderWorldSpacePos << 4
                                        | m_GerstnerWave << 3
                                        | m_CircularWavesEnabled << 2
                                        | m_DebugRenderNormals << 1
                                        | m_SineWave);

    for (int WaveIndex = 0; WaveIndex < SampleSceneData.WaveCount; WaveIndex++) {

        WaveData *WaveGPU = &WaveGPUData[WaveIndex];
        WaveDataCPU *WaveCPU = &WaveCPUData[WaveIndex];

        WaveGPU->Direction.x = WaveCPU->Direction.x;
        WaveGPU->Direction.z = WaveCPU->Direction.y;
        WaveGPU->Direction.y = (2 * PI) / WaveCPU->Length;
        WaveGPU->Direction.w = WaveCPU->Speed * WaveGPU->Direction.y;
        WaveGPU->Amplitude = WaveCPU->Amplitude;
        WaveGPU->Steepness = WaveCPU->Steepness;
        WaveGPU->CircularWave = glm::vec4(WaveCPU->CircularWaveCenter, 0.0f, 0.0f);
    }

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], &SampleSceneData);
	gfxDevice->UpdateBuffer(m_WavesBuffer, &WaveGPUData);
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

    RenderLightSource(currentFrame, commandBuffer, &m_LightSourcePSO);

	m_OffscreenRenderTarget->End(commandBuffer);

	m_OffscreenRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_OffscreenRenderTarget->GetColorBuffer());
}

void OceanRendering::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

    ImGui::Checkbox("Render Wireframe",				&m_RenderWireframe);

    const float currentAverageWaveLength = m_AverageWaveLength;
    ImGui::DragFloat("Average wave length",         &m_AverageWaveLength, 0.5f, 0.0f, 50.0f);

    const float currentAverageWaveAmplitude = m_AverageWaveAmplitude;
    ImGui::DragFloat("Average wave amplitude",      &m_AverageWaveAmplitude, 0.01f, 0.0001f, 50.0f);

    const float currentGravitationalConstant = m_GravitationalConstant;
    ImGui::DragFloat("Gravitational Constant",      &m_GravitationalConstant, 1.0f, 0.0f, 200.0f);

    const glm::vec2 currentAverageDirection = m_AverageWaveDirection;
    ImGui::DragFloat2("Average direction", (float*)&m_AverageWaveDirection, 0.01f, 0.0f, 1.0f);

    const float currentDirectionDeviation = m_DirectionDeviation;
    ImGui::DragFloat("Direction deviation", &m_DirectionDeviation, 0.01f, 0.1f, 1.0f);

    const float currentAverageWaveSteepness = m_AverageWaveSteepness;
    ImGui::DragFloat("Average wave steepness", &m_AverageWaveSteepness, 0.01f, 1.0f, 50.0f);

    const float currentWaveSteepnessDeviation = m_WaveSteepnessDeviation;
    ImGui::DragFloat("Wave steepness deviation", &m_WaveSteepnessDeviation, 0.01f, 0.0f, 15.0f);

    const int currentWaveCount = SampleSceneData.WaveCount;
    ImGui::DragInt("Wave Count", &SampleSceneData.WaveCount, 1, 1, static_cast<int>(SINE_WAVES_MAX));
    ImGui::DragFloat("Specular Displacement", &SampleSceneData.SpecularDisplacement, 0.01f, -10.0f, 10.0f);

    if (currentAverageWaveLength != m_AverageWaveLength 
        || currentAverageWaveAmplitude != m_AverageWaveAmplitude 
        || currentGravitationalConstant != m_GravitationalConstant
        || currentAverageDirection != m_AverageWaveDirection
        || currentDirectionDeviation != m_DirectionDeviation
        || currentWaveSteepnessDeviation != m_WaveSteepnessDeviation
        || currentAverageWaveSteepness != m_AverageWaveSteepness) {
        GenerateWaveData(); 
    }

    ImGui::Checkbox("Circular Waves Enabled",       &m_CircularWavesEnabled);

    const bool originalSineWaveEnabled = m_SineWave;
    const bool originalGerstnerWaveEnabled = m_GerstnerWave;
    const bool originalSineWaveFractalBrownianMotionEnabled = m_SineWaveFractalBrownianMotion;
    
    ImGui::Checkbox("Sine Wave",        &m_SineWave);
    ImGui::Checkbox("Gerstner Wave",    &m_GerstnerWave);
    ImGui::Checkbox("Sine Wave Fractal Brownian Motion", &m_SineWaveFractalBrownianMotion);
    ImGui::Checkbox("Fractal Brownian Motion Domain Warping Enabled", &m_FractalBrownianMotionDomainWarpingEnabled);
    if (m_SineWave && !originalSineWaveEnabled) {
        m_GerstnerWave = false;
        m_SineWaveFractalBrownianMotion = false;
    }

    if (m_GerstnerWave && !originalGerstnerWaveEnabled) {
        m_SineWave = false;
        m_SineWaveFractalBrownianMotion = false;
    }

    if (m_SineWaveFractalBrownianMotion && !originalSineWaveFractalBrownianMotionEnabled) {
        m_SineWave = false;
        m_GerstnerWave = false;
    }

    if (m_SineWaveFractalBrownianMotion) {
        ImGui::DragFloat("Water Depth", &SampleSceneData.WaterDepth, 0.01f, 0.0f, 50.0f);
        ImGui::DragFloat("Sine FBM Wave Amplitude", &SampleSceneData.SineFBMAmplitude, 0.001f, 0.0f, 100.0f);
        ImGui::DragFloat("Sine FBM Wave Frequency", &SampleSceneData.SineFBMFrequency, 0.001f, 0.0f, 100.0f);
        ImGui::DragFloat("Sine FBM Wave Amplitude Multiplier (should be smaller than wave amplitude)", &SampleSceneData.SineFBMAmplitudeMultiplier, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Sine FBM Wave Frequency Multiplier (should be greater than wave frequency)", &SampleSceneData.SineFBMFrequencyMultiplier, 0.001f, 1.0f, 10.0f);
        ImGui::DragFloat("Drag Mult", &SampleSceneData.DragMult, 0.001f, 0.0f, 10.0f);
        ImGui::DragFloat("Wind Angle", &SampleSceneData.WindAngle, 0.01f, -90.0f, 90.0f);
        ImGui::DragFloat("Wind Speed", &SampleSceneData.WindSpeed, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Temporal Phase Exponent", &SampleSceneData.TemporalPhaseExponent, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Height Multiplier", &SampleSceneData.HeightMultiplier, 0.01f, 0.0f, 10.0f);
    }

    ImGui::Checkbox("Debug - Render World Space Pos",   &m_DebugRenderWorldSpacePos);
    ImGui::Checkbox("Debug - Render Normals",           &m_DebugRenderNormals);

    ImGui::DragFloat("Water Shininess",                 &SampleSceneData.WaterShininess, 1.0f, 0.0f, 3000.0f);

    ImGui::DragFloat3("Light Position",				    (float*)&SampleSceneData.LightPosition, 0.2f, -1000.0f, 1000.0f);
    ImGui::DragFloat("Light Strength",                  &SampleSceneData.LightPosition.w, 0.1f, 0.0f, 200.0f);
    ImGui::ColorPicker3("Light Color",                  (float*)&SampleSceneData.LightColor);
	ImGui::DragFloat("Light Specular",			        &SampleSceneData.LightColor.w, 0.02f, 0.0f, 1.0f);

    if (ImGui::Button("Print Active Wave CPU Data")) {
        PrintActiveWaveCPUData();
    }
    
    if (ImGui::TreeNode("Waves Settings")) {
        for (int WaveIndex = 0; WaveIndex < SINE_WAVES_MAX; WaveIndex++) { 
            std::string WaveId = "wave_" + std::to_string(WaveIndex);

            if (ImGui::TreeNode(WaveId.c_str())) {
                ImGui::DragFloat("Wave Length",                 &WaveCPUData[WaveIndex].Length, 0.001f, 0.0f, 100.0f);
                ImGui::DragFloat("Wave Amplitude",              &WaveCPUData[WaveIndex].Amplitude, 0.001f, 0.0f, 10.00f);
                ImGui::DragFloat("Wave Steepness",              &WaveCPUData[WaveIndex].Steepness, 0.01, 1.0f, 10.0f);
                ImGui::DragFloat("Wave Speed",                  &WaveCPUData[WaveIndex].Speed, 0.001f, 0.0f, 10.00f);
                ImGui::DragFloat2("Wave Direction (X and Z)",   (float*)&WaveCPUData[WaveIndex].Direction, 0.001f, -1.0f, 1.0f);
                ImGui::DragFloat2("Circular Wave Center (X and Z)",   (float*)&WaveCPUData[WaveIndex].CircularWaveCenter, 0.1f, -200.0f, 200.0f);

                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    m_WaterModel->OnUIRender();
}

void OceanRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

RUN_APPLICATION(OceanRendering);
