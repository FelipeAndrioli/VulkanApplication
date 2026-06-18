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
        alignas(4) float Amplitude = 0.0f;
        alignas(4) float Steepness = 0.0f;
    } WaveGPUData[SINE_WAVES_MAX];

    struct WaveDataCPU {
        glm::vec2 Direction = glm::vec2(0.0f);
        float Length = 0.0;
        float Speed = 0.0f;
        float Amplitude = 0.0f;
        float Steepness = 0.0f;
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

//	glm::vec4 m_LightPosition = glm::vec4(1.0f, 327.0f, 1.0f, 1.0f);
	glm::vec4 m_LightPosition = glm::vec4(1.0f, 20.0f, 1.0f, 1.0f);
    glm::vec3 m_WaterColor = glm::vec3(0.09f, 0.55f, 0.79f);

    float m_OrbitalLightSpeed = 0.5f;
	float m_OrbitalLightDisplacement = 3.0f;
    float m_WaterSpecularFactor = 32.0f; 

    bool m_TessellationEnabled = false;
    bool m_OrbitateLight = false;
    bool m_RenderWireframe = false;
    bool m_SineWave = false;
    bool m_DebugRenderNormals = false;
    bool m_GenerateNormalPerFragment = false;
    bool m_GenerateRandomWaveData = false;

private:
    void RenderLightSource(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline);
    void Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline, VkDescriptorSet *frameDescriptorSet, bool drawIndexed = false);

    void PrintActiveWaveCPUData();
    void SetupTessellationHardCodedWaveData();
    void SetupHardCodedWaveData();
    void GenerateRandomWaveData();
};

void OceanRendering::PrintActiveWaveCPUData() {

    for (size_t waveIndex = 0; waveIndex < SampleSceneData.SineWaveCount; ++waveIndex) {
        const WaveDataCPU &wave = WaveCPUData[waveIndex];

        std::cout << "WaveCPUData[" << waveIndex << "].Direction.x = " << wave.Direction.x << "f;\n";
        std::cout << "WaveCPUData[" << waveIndex << "].Direction.y = " << wave.Direction.y << "f;\n";
        std::cout << "WaveCPUData[" << waveIndex << "].Length  = " << wave.Length << "f;\n";
        std::cout << "WaveCPUData[" << waveIndex << "].Speed = " << wave.Speed << "f;\n";
        std::cout << "WaveCPUData[" << waveIndex << "].Amplitude = " << wave.Amplitude << "f;\n";
        std::cout << "WaveCPUData[" << waveIndex << "].Steepness = " << wave.Steepness << "f;\n";
    }
}

void OceanRendering::GenerateRandomWaveData() {
    std::uniform_real_distribution<float> randomAmplitude(0.1f, 1.0f);
    std::uniform_real_distribution<float> randomLength(0.1f, 1.0f);
    std::uniform_real_distribution<float> randomSpeed(0.1f, 1.0f);
    std::uniform_real_distribution<float> randomSteepness(0.0f, 1.0f);
    std::uniform_real_distribution<float> randomDirection(-1.0f, 1.0f);

    std::default_random_engine generator;

    for (size_t waveIndex = 0; waveIndex < SINE_WAVES_MAX; ++waveIndex) {
        WaveCPUData[waveIndex].Amplitude = randomAmplitude(generator);
        WaveCPUData[waveIndex].Length = randomLength(generator);
        WaveCPUData[waveIndex].Speed = randomSpeed(generator);
        WaveCPUData[waveIndex].Steepness = randomSteepness(generator);
        WaveCPUData[waveIndex].Direction.x = randomDirection(generator);
        WaveCPUData[waveIndex].Direction.y = randomDirection(generator);
    }
}

void OceanRendering::SetupHardCodedWaveData() {
    // big base waves
    WaveCPUData[0].Direction.x = 0.80484f;  WaveCPUData[0].Direction.y = 0.25133f;  WaveCPUData[0].Length = 25.00000f; WaveCPUData[0].Speed = 7.51f;  WaveCPUData[0].Amplitude = 0.75000f;
    WaveCPUData[1].Direction.x = 0.71344f;  WaveCPUData[1].Direction.y = 0.22806f;  WaveCPUData[1].Length = 27.55000f; WaveCPUData[1].Speed = 7.82f;  WaveCPUData[1].Amplitude = 0.95461f;
    WaveCPUData[2].Direction.x = 0.60789f;  WaveCPUData[2].Direction.y = 0.20874f;  WaveCPUData[2].Length = 30.10000f; WaveCPUData[2].Speed = 8.12f;  WaveCPUData[2].Amplitude = 1.18293f;
    WaveCPUData[3].Direction.x = 0.72594f;  WaveCPUData[3].Direction.y = 0.19244f;  WaveCPUData[3].Length = 32.65000f; WaveCPUData[3].Speed = 8.41f;  WaveCPUData[3].Amplitude = 1.43497f;
    WaveCPUData[4].Direction.x = 0.62208f;  WaveCPUData[4].Direction.y = 0.17850f;  WaveCPUData[4].Length = 35.20000f; WaveCPUData[4].Speed = 8.70f;  WaveCPUData[4].Amplitude = 1.18272f;
    WaveCPUData[5].Direction.x = 0.73820f;  WaveCPUData[5].Direction.y = 0.16644f;  WaveCPUData[5].Length = 37.75000f; WaveCPUData[5].Speed = 8.98f;  WaveCPUData[5].Amplitude = 1.44394f;

    // medium waves
    WaveCPUData[6].Direction.x = 0.50501f;  WaveCPUData[6].Direction.y = 1.00692f;  WaveCPUData[6].Length = 6.24000f;  WaveCPUData[6].Speed = 9.25f;  WaveCPUData[6].Amplitude = 0.23650f;
    WaveCPUData[7].Direction.x = 0.81539f;  WaveCPUData[7].Direction.y = 0.75884f;  WaveCPUData[7].Length = 8.28000f;  WaveCPUData[7].Speed = 9.11f;  WaveCPUData[7].Amplitude = 0.22811f;
    WaveCPUData[8].Direction.x = 0.54585f;  WaveCPUData[8].Direction.y = 0.60884f;  WaveCPUData[8].Length = 10.32000f; WaveCPUData[8].Speed = 8.95f;  WaveCPUData[8].Amplitude = 0.33230f;
    WaveCPUData[9].Direction.x = 0.84223f;  WaveCPUData[9].Direction.y = 0.50835f;  WaveCPUData[9].Length = 12.36000f; WaveCPUData[9].Speed = 8.82f;  WaveCPUData[9].Amplitude = 0.45547f;
    WaveCPUData[10].Direction.x = 0.58542f; WaveCPUData[10].Direction.y = 0.43633f; WaveCPUData[10].Length = 14.40000f; WaveCPUData[10].Speed = 8.52f;  WaveCPUData[10].Amplitude = 0.38160f;
    WaveCPUData[11].Direction.x = 0.86712f; WaveCPUData[11].Direction.y = 0.38219f; WaveCPUData[11].Length = 16.44000f; WaveCPUData[11].Speed = 8.44f;  WaveCPUData[11].Amplitude = 0.51211f;
    WaveCPUData[12].Direction.x = 0.62365f; WaveCPUData[12].Direction.y = 0.96963f; WaveCPUData[12].Length = 6.48000f;  WaveCPUData[12].Speed = 9.21f;  WaveCPUData[12].Amplitude = 0.23198f;
    WaveCPUData[13].Direction.x = 0.89002f; WaveCPUData[13].Direction.y = 0.73746f; WaveCPUData[13].Length = 8.52000f;  WaveCPUData[13].Speed = 9.05f;  WaveCPUData[13].Amplitude = 0.21683f;
    WaveCPUData[14].Direction.x = 0.66044f; WaveCPUData[14].Direction.y = 0.59500f; WaveCPUData[14].Length = 10.56000f; WaveCPUData[14].Speed = 8.91f;  WaveCPUData[14].Amplitude = 0.31786f;
    WaveCPUData[15].Direction.x = 0.91087f; WaveCPUData[15].Direction.y = 0.49867f; WaveCPUData[15].Length = 12.60000f; WaveCPUData[15].Speed = 8.78f;  WaveCPUData[15].Amplitude = 0.43785f;
    WaveCPUData[16].Direction.x = 0.69570f; WaveCPUData[16].Direction.y = 0.42918f; WaveCPUData[16].Length = 14.64000f; WaveCPUData[16].Speed = 8.61f;  WaveCPUData[16].Amplitude = 0.57682f;
    WaveCPUData[17].Direction.x = 0.38333f; WaveCPUData[17].Direction.y = 0.37669f; WaveCPUData[17].Length = 16.68000f; WaveCPUData[17].Speed = 8.35f;  WaveCPUData[17].Amplitude = 0.48455f;
    WaveCPUData[18].Direction.x = 0.72937f; WaveCPUData[18].Direction.y = 0.93500f; WaveCPUData[18].Length = 6.72000f;  WaveCPUData[18].Speed = 9.10f;  WaveCPUData[18].Amplitude = 0.22646f;
    WaveCPUData[19].Direction.x = 0.42720f; WaveCPUData[19].Direction.y = 0.71726f; WaveCPUData[19].Length = 8.76000f;  WaveCPUData[19].Speed = 8.90f;  WaveCPUData[19].Amplitude = 0.33595f;
    WaveCPUData[20].Direction.x = 0.76135f; WaveCPUData[20].Direction.y = 0.58178f; WaveCPUData[20].Length = 10.80000f; WaveCPUData[20].Speed = 8.72f;  WaveCPUData[20].Amplitude = 0.30240f;
    WaveCPUData[21].Direction.x = 0.47009f; WaveCPUData[21].Direction.y = 0.48934f; WaveCPUData[21].Length = 12.84000f; WaveCPUData[21].Speed = 8.59f;  WaveCPUData[21].Amplitude = 0.41923f;

    // fast waves on surface
    WaveCPUData[22].Direction.x = 0.85492f; WaveCPUData[22].Direction.y = 1.68903f; WaveCPUData[22].Length = 3.72000f;  WaveCPUData[22].Speed = 10.31f; WaveCPUData[22].Amplitude = 0.13541f;
    WaveCPUData[23].Direction.x = 0.31494f; WaveCPUData[23].Direction.y = 1.48539f; WaveCPUData[23].Length = 4.23000f;  WaveCPUData[23].Speed = 9.94f;  WaveCPUData[23].Amplitude = 0.09560f;
    WaveCPUData[24].Direction.x = 0.89809f; WaveCPUData[24].Direction.y = 3.61103f; WaveCPUData[24].Length = 1.74000f;  WaveCPUData[24].Speed = 12.55f; WaveCPUData[24].Amplitude = 0.05011f;
    WaveCPUData[25].Direction.x = 0.39897f; WaveCPUData[25].Direction.y = 2.79253f; WaveCPUData[25].Length = 2.25000f;  WaveCPUData[25].Speed = 11.82f; WaveCPUData[25].Amplitude = 0.07875f;
    WaveCPUData[26].Direction.x = 0.93398f; WaveCPUData[26].Direction.y = 2.27652f; WaveCPUData[26].Length = 2.76000f;  WaveCPUData[26].Speed = 11.23f; WaveCPUData[26].Amplitude = 0.05851f;
    WaveCPUData[27].Direction.x = 0.47978f; WaveCPUData[27].Direction.y = 1.92146f; WaveCPUData[27].Length = 3.27000f;  WaveCPUData[27].Speed = 10.80f; WaveCPUData[27].Amplitude = 0.08960f;
    WaveCPUData[28].Direction.x = 0.96232f; WaveCPUData[28].Direction.y = 1.66222f; WaveCPUData[28].Length = 3.78000f;  WaveCPUData[28].Speed = 10.45f; WaveCPUData[28].Amplitude = 0.12701f;
    WaveCPUData[29].Direction.x = 0.55669f; WaveCPUData[29].Direction.y = 1.46461f; WaveCPUData[29].Length = 4.29000f;  WaveCPUData[29].Speed = 10.15f; WaveCPUData[29].Amplitude = 0.17074f;
    WaveCPUData[30].Direction.x = 0.98286f; WaveCPUData[30].Direction.y = 3.49066f; WaveCPUData[30].Length = 1.80000f;  WaveCPUData[30].Speed = 12.33f; WaveCPUData[30].Amplitude = 0.04680f;
    WaveCPUData[31].Direction.x = 0.62910f; WaveCPUData[31].Direction.y = 2.71999f; WaveCPUData[31].Length = 2.31000f;  WaveCPUData[31].Speed = 11.66f; WaveCPUData[31].Amplitude = 0.07438f;

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

    m_TestWaterModel = ModelLoader::LoadMultiQuadModel(200, 200);
	m_TestWaterModel->ModelIndex = 1;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/OceanRendering/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, m_TessellationControlShader, "../src/Samples/OceanRendering/tessellation_control.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_TessellationEvaluationShader, "../src/Samples/OceanRendering/tessellation_evaluation.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/OceanRendering/fragment.glsl");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(SceneData));
	m_SineWavesBuffer = gfxDevice->CreateBuffer(sizeof(WaveData) * SINE_WAVES_MAX);

    if (m_GenerateRandomWaveData) {
        GenerateRandomWaveData();
    } else if (m_TessellationEnabled) {
        SetupTessellationHardCodedWaveData();
    } else {
        SetupHardCodedWaveData();
    }

    SampleSceneData.SineWaveCount = 1;

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
        WaveGPU->Steepness = WaveCPU->Steepness;
    }

	gfxDevice->UpdateBuffer(m_SceneBuffer, &SampleSceneData);
	gfxDevice->UpdateBuffer(m_SineWavesBuffer, &WaveGPUData);
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

    ImGui::ColorPicker3("Water Color",              (float*)&m_WaterColor);
    ImGui::DragFloat("Water Specular factor",       &m_WaterSpecularFactor, 2.0f, 0.0f, 64.0f);
    ImGui::Checkbox("Render Wireframe",				&m_RenderWireframe);

    const bool tessellationEnabled = m_TessellationEnabled;

    ImGui::Checkbox("Tessellation Enabled",         &m_TessellationEnabled);

    if (tessellationEnabled != m_TessellationEnabled && !tessellationEnabled) {
        SetupTessellationHardCodedWaveData();
    }

    if (tessellationEnabled != m_TessellationEnabled && tessellationEnabled) {
        SetupHardCodedWaveData();
    }

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
	ImGui::DragFloat4("Light Position",				(float*)&m_LightPosition, 0.2f, -1000.0f, 1000.0f);

    ImGui::DragInt("Active Sine Waves", &SampleSceneData.SineWaveCount, 1, 1, static_cast<int>(SINE_WAVES_MAX));

    if (ImGui::Button("Print Active Wave CPU Data")) {
        PrintActiveWaveCPUData();
    }
    
    if (ImGui::TreeNode("Sine Waves Settings")) {
        for (int WaveIndex = 0; WaveIndex < SINE_WAVES_MAX; WaveIndex++) {
            std::string WaveId = "wave_" + std::to_string(WaveIndex);

            if (ImGui::TreeNode(WaveId.c_str())) {
                ImGui::DragFloat("Wave Length",                 &WaveCPUData[WaveIndex].Length, 0.001f, 0.0f, 100.0f);
                ImGui::DragFloat("Wave Amplitude",              &WaveCPUData[WaveIndex].Amplitude, 0.001f, 0.0f, 10.00f);
                ImGui::DragFloat("Wave Steepness",              &WaveCPUData[WaveIndex].Steepness, 1.0, 0.0f, 50.00f);
                ImGui::DragFloat("Wave Speed",                  &WaveCPUData[WaveIndex].Speed, 0.001f, 0.0f, 10.00f);
                ImGui::DragFloat2("Wave Direction (X and Z)",   (float*)&WaveCPUData[WaveIndex].Direction, 0.001f, -1.0f, 1.0f);

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
