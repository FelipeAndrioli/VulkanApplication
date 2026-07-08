#include <iostream>
#include <random>

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

    // TODO: organize mess with UBO
    // TODO: get rid of sine/gerstner/tessellated pipelines
    // TODO: hardcode fixed parameters in the shader
    // TODO: add tessellation to the final pipeline
	struct SceneData {
		alignas(16) glm::mat4 Projection;
		alignas(16) glm::mat4 View;
		alignas(16) glm::vec4 LightPosition;
		alignas(16) glm::vec4 LightColor;       // w is specular
		alignas(16) glm::vec4 ViewerPosition;
        // Note: W is specular factor
		alignas(16) glm::vec4 DeepWaterColor = glm::vec4(0.0293f, 0.0698f, 0.1717f, 1.0f);
		alignas(16) glm::vec4 ShallowWaterColor = glm::vec4(0.1529f, 0.8901f, 0.8392f, 16.0f);
        alignas(4) int Flags;
        alignas(4) int WaveCount = 1;
        alignas(4) float ShallowWaterColorSumDeviation = 0.0f;
        alignas(4) float DeepWaterColorSumDeviation = 0.0f;
        alignas(4) float Time = 0.0f;
        alignas(4) float WaterDepth = 6.7f;
        alignas(4) float SineFBMAmplitude = 1.200;
        alignas(4) float SineFBMFrequency = 0.215f;
        alignas(4) float SineFBMAmplitudeMultiplier = 0.770f;   // must be smaller than 1.0
        alignas(4) float SineFBMFrequencyMultiplier = 1.275;    // must be greater than 1.0
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

	std::shared_ptr<Assets::Model> m_OceanModel;
    std::shared_ptr<Assets::Model> m_TestWaterModel;

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_OffscreenRenderTarget;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_TessellationControlShader = {};
	Graphics::Shader m_TessellationEvaluationShader = {};
	Graphics::Shader m_FragShader = {};
    Graphics::Shader m_VertexShaderTessellationDisabled = {};

	Graphics::Buffer m_SceneBuffer = {};
    Graphics::Buffer m_WavesBuffer = {};

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

//	glm::vec4 m_LightPosition = glm::vec4(1.0f, 327.0f, 1.0f, 1.0f);
	glm::vec4 m_LightPosition = glm::vec4(1.0f, 20.0f, 1.0f, 1.0f);

    glm::vec2 m_AverageWaveDirection = glm::vec2(1.0f, 0.0f);

    float m_OrbitalLightSpeed = 0.5f;
	float m_OrbitalLightDisplacement = 3.0f;
    float m_AverageWaveLength = 7.0f;
    float m_AverageWaveAmplitude = 0.110f;
    float m_GravitationalConstant = 9.8f; // m/s^2
    float m_DirectionDeviation = 0.170f; 
    float m_AverageWaveSteepness = 11.0f;
    float m_WaveSteepnessDeviation = 2.4f;

    bool m_TessellationEnabled = false;
    bool m_OrbitateLight = false;
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
    void Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet, bool drawIndexed = false);

    void PrintActiveWaveCPUData();
    void SetupTessellationHardCodedWaveData();
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

void OceanRendering::SetupTessellationHardCodedWaveData() {
    WaveCPUData[0].Direction.x = 0.731f;
    WaveCPUData[0].Direction.y = 0.0f;
    WaveCPUData[0].Length  = 0.08f;
    WaveCPUData[0].Speed = 0.417f;
    WaveCPUData[0].Amplitude = 0.005f;
    WaveCPUData[0].Steepness = 0.835009f;
    WaveCPUData[1].Direction.x = -0.804919f;
    WaveCPUData[1].Direction.y = 0.0944412f;
    WaveCPUData[1].Length  = 0.163f;
    WaveCPUData[1].Speed = 0.147f;
    WaveCPUData[1].Amplitude = 0.002f;
    WaveCPUData[1].Steepness = 0.308167f;
    WaveCPUData[2].Direction.x = 0.915014f;
    WaveCPUData[2].Direction.y = 0.992923f;
    WaveCPUData[2].Length  = 0.539088f;
    WaveCPUData[2].Speed = 0.121f;
    WaveCPUData[2].Amplitude = 0.006f;
    WaveCPUData[2].Steepness = 0.992881f;
    WaveCPUData[3].Direction.x = 0.941185f;
    WaveCPUData[3].Direction.y = 0.962219f;
    WaveCPUData[3].Length  = 0.417f;
    WaveCPUData[3].Speed = 0.157613f;
    WaveCPUData[3].Amplitude = 0.004f;
    WaveCPUData[3].Steepness = 0.725839f;
    WaveCPUData[4].Direction.x = 0.600561f;
    WaveCPUData[4].Direction.y = -0.405941f;
    WaveCPUData[4].Length  = 0.09f;
    WaveCPUData[4].Speed = 0.184f;
    WaveCPUData[4].Amplitude = 0.002f;
    WaveCPUData[4].Steepness = 0.798106f;
    WaveCPUData[5].Direction.x = 0.831471f;
    WaveCPUData[5].Direction.y = 0.279527f;
    WaveCPUData[5].Length  = 0.20861f;
    WaveCPUData[5].Speed = 0.076f;
    WaveCPUData[5].Amplitude = 0.003f;
    WaveCPUData[5].Steepness = 0.112464f;
    WaveCPUData[6].Direction.x = 0.311481f;
    WaveCPUData[6].Direction.y = 0.595857f;
    WaveCPUData[6].Length  = 0.339f;
    WaveCPUData[6].Speed = 0.13f;
    WaveCPUData[6].Amplitude = 0.005f;
    WaveCPUData[6].Steepness = 0.503663f;
    WaveCPUData[7].Direction.x = 0.867986f;
    WaveCPUData[7].Direction.y = 0.362719f;
    WaveCPUData[7].Length  = 0.328f;
    WaveCPUData[7].Speed = 0.836f;
    WaveCPUData[7].Amplitude = 0.00357116f;
    WaveCPUData[7].Steepness = 0.211924f;
    WaveCPUData[8].Direction.x = 0.486265f;
    WaveCPUData[8].Direction.y = -0.0504827f;
    WaveCPUData[8].Length  = 0.073f;
    WaveCPUData[8].Speed = 0.183f;
    WaveCPUData[8].Amplitude = 0.004f;
    WaveCPUData[8].Steepness = 0.740647f;
    WaveCPUData[9].Direction.x = -0.657627f;
    WaveCPUData[9].Direction.y = -0.396174f;
    WaveCPUData[9].Length  = 0.242f;
    WaveCPUData[9].Speed = 0.213f;
    WaveCPUData[9].Amplitude = 0.007f;
    WaveCPUData[9].Steepness = 0.173865f;
    WaveCPUData[10].Direction.x = -0.446154f;
    WaveCPUData[10].Direction.y = 0.744858f;
    WaveCPUData[10].Length  = 0.468f;
    WaveCPUData[10].Speed = 0.0318328f;
    WaveCPUData[10].Amplitude = 0.01f;
    WaveCPUData[10].Steepness = 0.31655f;
    WaveCPUData[11].Direction.x = 0.646916f;
    WaveCPUData[11].Direction.y = 0.643806f;
    WaveCPUData[11].Length  = 0.468405f;
    WaveCPUData[11].Speed = 0.0971317f;
    WaveCPUData[11].Amplitude = 0.00461714f;
    WaveCPUData[11].Steepness = 0.994068f;
    WaveCPUData[12].Direction.x = 0.900444f;
    WaveCPUData[12].Direction.y = -0.018822f;
    WaveCPUData[12].Length  = 0.226f;
    WaveCPUData[12].Speed = 0.317099f;
    WaveCPUData[12].Amplitude = 0.008f;
    WaveCPUData[12].Steepness = 0.76375f;
    WaveCPUData[13].Direction.x = -0.236883f;
    WaveCPUData[13].Direction.y = -0.579582f;
    WaveCPUData[13].Length  = 1.112f;
    WaveCPUData[13].Speed = 0.438744f;
    WaveCPUData[13].Amplitude = 0.00344461f;
    WaveCPUData[13].Steepness = 0.125897f;
    WaveCPUData[14].Direction.x = -0.626255f;
    WaveCPUData[14].Direction.y = -0.182538f;
    WaveCPUData[14].Length  = 0.29219f;
    WaveCPUData[14].Speed = 0.233f;
    WaveCPUData[14].Amplitude = 0.009f;
    WaveCPUData[14].Steepness = 0.0364412f;
    WaveCPUData[15].Direction.x = 0.292626f;
    WaveCPUData[15].Direction.y = 0.58795f;
    WaveCPUData[15].Length  = 0.14f;
    WaveCPUData[15].Speed = 0.253f;
    WaveCPUData[15].Amplitude = 0.002f;
    WaveCPUData[15].Steepness = 0.487569f;
    WaveCPUData[16].Direction.x = -0.44795f;
    WaveCPUData[16].Direction.y = 0.411548f;
    WaveCPUData[16].Length  = 0.438f;
    WaveCPUData[16].Speed = 0.327f;
    WaveCPUData[16].Amplitude = 0.013f;
    WaveCPUData[16].Steepness = 0.807531f;
    WaveCPUData[17].Direction.x = -0.674777f;
    WaveCPUData[17].Direction.y = 0.287922f;
    WaveCPUData[17].Length  = 0.333f;
    WaveCPUData[17].Speed = 0.083f;
    WaveCPUData[17].Amplitude = 0.006f;
    WaveCPUData[17].Steepness = 0.710704f;
    WaveCPUData[18].Direction.x = 0.919488f;
    WaveCPUData[18].Direction.y = 0.147509f;
    WaveCPUData[18].Length  = 0.74f;
    WaveCPUData[18].Speed = 0.229f;
    WaveCPUData[18].Amplitude = 0.0118998f;
    WaveCPUData[18].Steepness = 0.773917f;
    WaveCPUData[19].Direction.x = -0.552376f;
    WaveCPUData[19].Direction.y = -0.964452f;
    WaveCPUData[19].Length  = 0.78f;
    WaveCPUData[19].Speed = 0.258f;
    WaveCPUData[19].Amplitude = 0.023f;
    WaveCPUData[19].Steepness = 0.808175f;
    WaveCPUData[20].Direction.x = 0.011914f;
    WaveCPUData[20].Direction.y = 0.880148f;
    WaveCPUData[20].Length  = 0.128f;
    WaveCPUData[20].Speed = 0.255095f;
    WaveCPUData[20].Amplitude = 0.005f;
    WaveCPUData[20].Steepness = 0.820841f;
    WaveCPUData[21].Direction.x = 0.918583f;
    WaveCPUData[21].Direction.y = 0.161913f;
    WaveCPUData[21].Length  = 0.436f;
    WaveCPUData[21].Speed = 0.104f;
    WaveCPUData[21].Amplitude = 0.007f;
    WaveCPUData[21].Steepness = 0.423165f;
    WaveCPUData[22].Direction.x = -0.701412f;
    WaveCPUData[22].Direction.y = -0.539688f;
    WaveCPUData[22].Length  = 0.245f;
    WaveCPUData[22].Speed = 0.138624f;
    WaveCPUData[22].Amplitude = 0.005f;
    WaveCPUData[22].Steepness = 0.761731f;
    WaveCPUData[23].Direction.x = -0.491436f;
    WaveCPUData[23].Direction.y = -0.335104f;
    WaveCPUData[23].Length  = 0.229f;
    WaveCPUData[23].Speed = 0.213f;
    WaveCPUData[23].Amplitude = 0.003f;
    WaveCPUData[23].Steepness = 0.988522f;
    WaveCPUData[24].Direction.x = 0.858527f;
    WaveCPUData[24].Direction.y = -0.565524f;
    WaveCPUData[24].Length  = 0.167f;
    WaveCPUData[24].Speed = 0.507f;
    WaveCPUData[24].Amplitude = 0.002f;
    WaveCPUData[24].Steepness = 0.0135391f;
    WaveCPUData[25].Direction.x = -0.497832f;
    WaveCPUData[25].Direction.y = 0.910035f;
    WaveCPUData[25].Length  = 0.769f;
    WaveCPUData[25].Speed = 0.196595f;
    WaveCPUData[25].Amplitude = 0.008f;
    WaveCPUData[25].Steepness = 0.848468f;
    WaveCPUData[26].Direction.x = -0.296681f;
    WaveCPUData[26].Direction.y = -0.864809f;
    WaveCPUData[26].Length  = 0.191f;
    WaveCPUData[26].Speed = 0.102f;
    WaveCPUData[26].Amplitude = 0.005f;
    WaveCPUData[26].Steepness = 0.98746f;
    WaveCPUData[27].Direction.x = 0.0994471f;
    WaveCPUData[27].Direction.y = 0.465597f;
    WaveCPUData[27].Length  = 0.141f;
    WaveCPUData[27].Speed = 0.144f;
    WaveCPUData[27].Amplitude = 0.012f;
    WaveCPUData[27].Steepness = 0.594504f;
    WaveCPUData[28].Direction.x = 0.5144f;
    WaveCPUData[28].Direction.y = -0.215359f;
    WaveCPUData[28].Length  = 0.477f;
    WaveCPUData[28].Speed = 0.285839f;
    WaveCPUData[28].Amplitude = 0.016f;
    WaveCPUData[28].Steepness = 0.67982f;
    WaveCPUData[29].Direction.x = 0.135643f;
    WaveCPUData[29].Direction.y = 0.0547428f;
    WaveCPUData[29].Length  = 0.071f;
    WaveCPUData[29].Speed = 0.117f;
    WaveCPUData[29].Amplitude = 0.009f;
    WaveCPUData[29].Steepness = 0.208068f;
    WaveCPUData[30].Direction.x = 0.0615951f;
    WaveCPUData[30].Direction.y = 0.185648f;
    WaveCPUData[30].Length  = 0.927575f;
    WaveCPUData[30].Speed = 0.0539501f;
    WaveCPUData[30].Amplitude = 0.00758542f;
    WaveCPUData[30].Steepness = 0.352762f;
    WaveCPUData[31].Direction.x = -0.740188f;
    WaveCPUData[31].Direction.y = -0.691123f;
    WaveCPUData[31].Length  = 0.1f;
    WaveCPUData[31].Speed = 0.175f;
    WaveCPUData[31].Amplitude = 0.006f;
    WaveCPUData[31].Steepness = 0.964966f;
}

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
    m_Camera.Far = 2000.0f;
    m_Camera.MovementSpeed = 0.1f;

	m_OceanModel = ModelLoader::LoadModel(ModelType::QUAD);
	m_OceanModel->Transformations.translation.y = -0.51f;
//	m_OceanModel->Transformations.rotation.x = 90.0f;
//	m_OceanModel->Transformations.scaleHandler = 20.0f;
//	m_OceanModel->Transformations.scaleHandler = 1000.0f;
	m_OceanModel->Transformations.scaleHandler = 100.0f;
	m_OceanModel->ModelIndex = 0;

    m_TestWaterModel = ModelLoader::LoadMultiQuadModel(150, 150, glm::vec3(0.0f), 0.5f);
//    m_TestWaterModel = ModelLoader::LoadMultiQuadModel(300, 300, glm::vec3(0.0f), 0.5f);
	m_TestWaterModel->ModelIndex = 1;

    SampleSceneData.LightColor = glm::vec4(1.0f);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/OceanRendering/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, m_TessellationControlShader, "../src/Samples/OceanRendering/tessellation_control.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_TessellationEvaluationShader, "../src/Samples/OceanRendering/tessellation_evaluation.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/OceanRendering/fragment.glsl");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(SceneData));
	m_WavesBuffer = gfxDevice->CreateBuffer(sizeof(WaveData) * SINE_WAVES_MAX);

    if (m_TessellationEnabled) {
        SetupTessellationHardCodedWaveData();
    } else {
        GenerateWaveData();
    }

    SampleSceneData.WaveCount = 1;

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
		gfxDevice->WriteDescriptor(frameInputLayout.bindings[1], m_FrameDescriptorSet[i], m_WavesBuffer);
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
        gfxDevice->WriteDescriptor(frameTessellationDisabledInputLayout.bindings[1], m_FrameTessellationDisabledDescriptorSet[i], m_WavesBuffer);
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

    m_OceanModel->OnUpdate(deltaT);
    m_TestWaterModel->OnUpdate(deltaT);

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

	gfxDevice->UpdateBuffer(m_SceneBuffer, &SampleSceneData);
	gfxDevice->UpdateBuffer(m_WavesBuffer, &WaveGPUData);
}

void OceanRendering::Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet, bool drawIndexed) {
	SCOPED_PROFILER_US("OceanRendering::Render");

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    gfxDevice->BindDescriptorSet(*frameDescriptorSet, commandBuffer, pipeline->pipelineLayout, 0, 1);

    Assets::Model& model = drawIndexed ? *m_TestWaterModel.get() : *m_OceanModel.get();

    VkDeviceSize offsets[] = { sizeof(uint32_t) * model.TotalIndices };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.DataBuffer.Handle, offsets);
    vkCmdBindIndexBuffer(commandBuffer, model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

    FramePushConstants.Model = model.GetModelMatrix();

    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);

    for (const auto& mesh: model.Meshes) { 
        if (drawIndexed) {
            vkCmdDrawIndexed(
                commandBuffer, 
                static_cast<uint32_t>(mesh.Indices.size()), 
                1, 
                static_cast<uint32_t>(mesh.IndexOffset), 
                static_cast<int32_t>(mesh.VertexOffset),
                0);
        } else {
            // Note: Must use vkCmdDraw instead of vkCmdDrawIndexed to tessellate quads.
            vkCmdDraw(commandBuffer, mesh.Vertices.size(), mesh.Indices.size(), 0, 0);
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

    ImGui::ColorPicker3("Deep Water Color",              (float*)&SampleSceneData.DeepWaterColor);
    ImGui::DragFloat("Deep Water Color Deviation",       &SampleSceneData.DeepWaterColor.a, 0.01f, -10.0f, 10.0f);
    ImGui::DragFloat("Deep Water Sum Color Deviation",   &SampleSceneData.DeepWaterColorSumDeviation, 0.1f, -10.0f, 10.0f);

    ImGui::ColorPicker3("Shallow Water Color",              (float*)&SampleSceneData.ShallowWaterColor);
    ImGui::DragFloat("Shallow Water Color Deviation",       &SampleSceneData.ShallowWaterColor.a, 0.01f, -10.0f, 10.0f);
    ImGui::DragFloat("Shallow Water Sum Color Deviation",   &SampleSceneData.ShallowWaterColorSumDeviation, 0.1f, -10.0f, 10.0f);

    ImGui::Checkbox("Render Wireframe",				&m_RenderWireframe);

    const bool tessellationEnabled = m_TessellationEnabled;

    ImGui::Checkbox("Tessellation Enabled",         &m_TessellationEnabled);

    if (tessellationEnabled != m_TessellationEnabled && !tessellationEnabled) {
        SetupTessellationHardCodedWaveData();
    }

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
    ImGui::DragInt("Active Waves", &SampleSceneData.WaveCount, 1, 1, static_cast<int>(SINE_WAVES_MAX));

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
    }

    ImGui::Checkbox("Debug - Render World Space Pos", &m_DebugRenderWorldSpacePos);
    ImGui::Checkbox("Debug - Render Normals",       &m_DebugRenderNormals);
	ImGui::Checkbox("Orbitate Light",				&m_OrbitateLight);
	ImGui::DragFloat("Light Orbital Speed",			&m_OrbitalLightSpeed, 0.02f, 0.0f, 3.0f);
	ImGui::DragFloat("Light Orbital Displacement",	&m_OrbitalLightDisplacement, 0.02f, 0.0f, 9.0f);
	ImGui::DragFloat4("Light Position",				(float*)&m_LightPosition, 0.2f, -1000.0f, 1000.0f);
    ImGui::ColorPicker3("Light Color",              (float*)&SampleSceneData.LightColor);
	ImGui::DragFloat("Light Specular",			    &SampleSceneData.LightColor.w, 0.02f, 0.0f, 10.0f);

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

    m_OceanModel->OnUIRender();
    m_TestWaterModel->OnUIRender();
}

void OceanRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
}

RUN_APPLICATION(OceanRendering);
