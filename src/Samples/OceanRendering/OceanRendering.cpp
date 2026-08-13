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

#define QUAD_GRID_VERTEX_COUNT 360 
//#define QUAD_GRID_VERTEX_COUNT 180
//#define QUAD_GRID_VERTEX_COUNT 30
#define QUAD_VERTEX_DISTANCE 8.0f
//#define QUAD_VERTEX_DISTANCE 5.0f
//#define QUAD_VERTEX_DISTANCE 1.0f

/*
    TODO's:
        - Move displacement calculation to compute shader
        - FFT for non-tiling water
        - Jacobian for foam 
        - BRDF
        - HDR bloom pass(?).
        - cinematic tone mapper for HDR bloom pass(?).
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

    struct SceneData {
		alignas(16) glm::mat4 Projection = glm::mat4(1.0f);
		alignas(16) glm::mat4 View = glm::mat4(1.0f);
		alignas(16) glm::vec4 LightPosition = glm::vec4(1500.0f, 225.0f, 264.0f, 0.8f);         // w is light strength
		alignas(16) glm::vec4 LightColor = glm::vec4(1.0f, 1.0f, 1.0f, 100.0f);                 // w is light specular factor
		alignas(16) glm::vec4 Sun = glm::vec4(0.0f, 0.0f, 0.07f, 1.14f);                        // xy -> pos; z -> radius; w -> strength
		alignas(16) glm::vec4 ViewerPosition = glm::vec4(0.0f);
		alignas(16) glm::vec4 WaterColor = glm::vec4(0.00858454f, 0.105058f, 0.0814091f, 0.94f);// w is ambient strength 
        alignas(16) glm::vec4 LocalSpaceCameraFrustumPlanes[6] = {}; 
        alignas(4) int Flags = 0;
        alignas(4) int WaveCount = WAVES_COUNT;
        alignas(4) int NormalWaveCount = WAVES_COUNT;
        alignas(4) float SpecularDisplacement = 1.002f;
        alignas(4) float WaterShininess = 1000.0f;
        alignas(4) float TemporalPhaseExponent = 0.8f;
        alignas(4) float HeightMultiplier = 1.0f;
        alignas(4) float WindAngle = 0.5f;
        alignas(4) float WindSpeed = 2.0f;
        alignas(4) float DragMult = 3.0f;
        alignas(4) float Time = 0.0f;
        alignas(4) float WaterDepth = 16.5f;
        alignas(4) float SineFBMAmplitude = 0.3f;
        alignas(4) float SineFBMFrequency = 0.05f;
        alignas(4) float SineFBMAmplitudeMultiplier = 0.780f; // must be smaller than 1.0
        alignas(4) float SineFBMFrequencyMultiplier = 1.315;  // must be greater than 1.0
        alignas(4) float TessellationMinThreshold = 40.0f;
        alignas(4) float TessellationMaxThreshold = 460.0f;
        alignas(4) float TessellationLevelMin = 1.0f;
        alignas(4) float TessellationLevelMax = 32.0f;
        alignas(4) float TessellationStep = 6.0f;
        alignas(4) float ReflectionStrength = 0.660f;
        alignas(4) float ImageWidth;
        alignas(4) float ImageHeight;
        alignas(4) float FogDensity = 0.003f;
        alignas(4) float FogHeightFalloff = 0.1f;
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
    uint32_t m_SampleCount = 0;

    Graphics::Texture m_SkyboxTexture = {};

    std::shared_ptr<Assets::Model> m_WaterModel;
    std::shared_ptr<Assets::Model> m_SkyboxCube;

    Graphics::RenderPassDescription m_OffscreenRenderPassDescription = {};
    Graphics::RenderPassDescription m_HDRPostEffectsRenderPassDescription = {};

    Graphics::GPUImage m_OffscreenPassColor = {};
    Graphics::GPUImage m_OffscreenPassResolvedColor = {};
    Graphics::GPUImage m_OffscreenDepth = {};
    Graphics::GPUImage m_OffscreenResolvedDepth = {};

    std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_OffscreenRenderTarget;
    std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_HDRPostProcessRenderTarget;
	std::unique_ptr<Graphics::PostEffectsRenderTarget> m_PostEffectsRenderTarget;

    Graphics::Shader m_VertexShader = {};
    Graphics::Shader m_TessellationControlShader = {};
    Graphics::Shader m_TessellationEvaluationShader = {};
	Graphics::Shader m_FragShader = {};

    Graphics::Shader m_HDRPostProcessFragmentShader = {};
    Graphics::PipelineState m_HDRPostProcessPSO = {};

	Graphics::GPUBuffer m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT] = {};

	Graphics::PipelineState m_DefaultPSO = {};
	Graphics::PipelineState m_WireframePSO = {};

    Graphics::InputLayout m_FrameInputLayout = {};
	VkDescriptorSetLayout m_FrameDescriptorSetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_FrameDescriptorSet = { VK_NULL_HANDLE };

    Graphics::Shader m_SkyboxVertexShader = {};
    Graphics::Shader m_SkyboxFragmentShader = {};
    Graphics::PipelineState m_SkyboxPSO = {};

    Graphics::Shader m_PostEffectsVertexShader = {};
    Graphics::Shader m_PostEffectsFragmentShader = {};
    Graphics::PipelineState m_PostEffectsPSO = {};

    glm::vec2 m_AverageWaveDirection = glm::vec2(1.0f, 0.0f);

    glm::mat4 m_WaterModelMatrix = glm::mat4(1.0f);

    float m_SkyboxCubeSize = 2000.0f;
    float m_SkyboxRotation = 234.0f;

    bool m_RenderWireframe = false;
    bool m_DebugRenderNormals = false;
    bool m_CircularWavesEnabled = false;
    bool m_DebugRenderWorldSpacePos = false;
    bool m_FractalBrownianMotionDomainWarpingEnabled = true;
    bool m_TessellationEnabled = true;
    bool m_ReflectionEnabled = true;
    bool m_RenderSkybox = true;
    bool m_RandomWaveDirectionEnabled = true;
    bool m_GenerateNormalPerFragment = false;
    bool m_GPUCullingFreeze = false;
private:

    void CreateDisplaySizeDependentResources(const uint32_t width, const uint32_t height);
    void RenderSkybox(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void RenderCube(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline);
    void RenderPostEffects(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline);
    void RenderModel(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame, const std::shared_ptr<Assets::Model>& model, const Graphics::PipelineState& pipeline, const PushConstants& pushContants) const;

    glm::vec2 CalculateScreenSpaceLightPos(const glm::mat4& Projection, const glm::mat4& View, const glm::vec3& WorldSpaceLightPos);
};

void OceanRendering::CreateDisplaySizeDependentResources(const uint32_t width, const uint32_t height) {

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    gfxDevice->DestroyImage(m_OffscreenPassColor);
    gfxDevice->DestroyImage(m_OffscreenDepth);
    gfxDevice->DestroyImage(m_OffscreenResolvedDepth);
    gfxDevice->DestroyImage(m_OffscreenPassResolvedColor);

    gfxDevice->CreateRenderTarget(
        m_OffscreenPassColor, 
        Graphics::Format::R32G32B32A32_FLOAT, 
        m_ScreenWidth, 
        m_ScreenHeight, 
        m_SampleCount);

    gfxDevice->CreateRenderTarget(
        m_OffscreenPassResolvedColor, 
        Graphics::Format::R32G32B32A32_FLOAT, 
        m_ScreenWidth, 
        m_ScreenHeight, 
        1);

    gfxDevice->CreateDepthOnlyBuffer(
        m_OffscreenDepth,
        { m_ScreenWidth, m_ScreenHeight },
        static_cast<VkSampleCountFlagBits>(m_SampleCount),
        1);

    gfxDevice->CreateImageSampler(m_OffscreenDepth);

    gfxDevice->CreateDepthOnlyBuffer(
        m_OffscreenResolvedDepth,
        { m_ScreenWidth, m_ScreenHeight },
        VK_SAMPLE_COUNT_1_BIT,
        1);
     
    gfxDevice->CreateImageSampler(m_OffscreenResolvedDepth);

    m_OffscreenRenderPassDescription.Attachments.clear();

    m_OffscreenRenderPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
            m_OffscreenPassColor, 
            gfxDevice->ConvertFormat(m_OffscreenPassColor.Description.Format),
            m_SampleCount,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED, 
            Graphics::ResourceState::RENDERTARGET, 
            Graphics::ResourceState::RENDERTARGET));

    m_OffscreenRenderPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::Depth(
            m_OffscreenDepth,
            gfxDevice->ConvertFormat(m_OffscreenDepth.Description.Format),
            m_SampleCount,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::DEPTH,
            Graphics::ResourceState::DEPTH_READONLY));

    m_OffscreenRenderPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::ResolveDepth(
            m_OffscreenResolvedDepth,
            gfxDevice->ConvertFormat(m_OffscreenResolvedDepth.Description.Format),
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::DEPTH,
            Graphics::ResourceState::DEPTH_READONLY));

    m_OffscreenRenderPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::Resolve(
            m_OffscreenPassResolvedColor,
            gfxDevice->ConvertFormat(m_OffscreenPassResolvedColor.Description.Format),
            1,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED, 
            Graphics::ResourceState::RENDERTARGET, 
            Graphics::ResourceState::RENDERTARGET));

    m_HDRPostEffectsRenderPassDescription.Attachments.clear();

    m_HDRPostEffectsRenderPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
            m_OffscreenPassResolvedColor,
            gfxDevice->ConvertFormat(m_OffscreenPassResolvedColor.Description.Format),
            1,
            Graphics::RenderPassAttachment::AttachmentLoadOp::LOAD,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::SHADER_RESOURCE));
}

void OceanRendering::RenderSkybox(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("OceanRendering::RenderSkybox");

    FramePushConstants.Color.r = m_SkyboxCubeSize;
    FramePushConstants.Color.g = glm::radians(m_SkyboxRotation);

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_SkyboxCubeSize));
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_SkyboxRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    
    FramePushConstants.Model = rotation * scale;

    vkCmdPushConstants(commandBuffer, m_SkyboxPSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);

    RenderCube(currentFrame, commandBuffer, &m_SkyboxPSO);
}

void OceanRendering::RenderCube(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline) {
	SCOPED_PROFILER_US("OceanRendering::RenderCube");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void OceanRendering::RenderPostEffects(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, Graphics::PipelineState *pipeline) {
    SCOPED_PROFILER_US("OceanRendering::RenderPostEffects");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

glm::vec2 OceanRendering::CalculateScreenSpaceLightPos(const glm::mat4& Projection, const glm::mat4& View, const glm::vec3& WorldSpaceLightPos) {

    glm::vec4 screenSpaceLightPos = Projection * View * glm::vec4(WorldSpaceLightPos.x, WorldSpaceLightPos.y, WorldSpaceLightPos.z, 1.0);
    screenSpaceLightPos /=  screenSpaceLightPos.w;

    return glm::vec2(screenSpaceLightPos.x, screenSpaceLightPos.y);
}

void OceanRendering::StartUp() {

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;
    m_SampleCount   = static_cast<uint32_t>(gfxDevice->GetMsaaSamples());

    CreateDisplaySizeDependentResources(m_ScreenWidth, m_ScreenHeight);

    m_OffscreenRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount,
        m_OffscreenRenderPassDescription);

    m_HDRPostProcessRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth,
        m_ScreenHeight,
        1,
        m_HDRPostEffectsRenderPassDescription);

    m_PostEffectsRenderTarget = std::make_unique<Graphics::PostEffectsRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);
    m_Camera.Far = 4000.0f;
    m_Camera.MovementSpeed = 0.1f;

    m_WaterModel = ModelLoader::LoadMultiQuadModel(QUAD_GRID_VERTEX_COUNT, QUAD_GRID_VERTEX_COUNT, glm::vec3(0.0f), QUAD_VERTEX_DISTANCE);
	m_WaterModel->ModelIndex = 0;

	m_FrameInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },	// Skybox texture
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },	// Offscreen pass color result texture
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },	// Offscreen pass depth result texture
		}
	};

    m_SkyboxTexture = TextureLoader::LoadCubemapTexture("./Textures/kloofendal_48d_partly_cloudy_puresky_8k.hdr");
    
	gfxDevice->CreateDescriptorSetLayout(m_FrameDescriptorSetLayout, m_FrameInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        m_SceneBuffer[i] = gfxDevice->CreateStorageBuffer(sizeof(SceneData));

        gfxDevice->CreateDescriptorSet(m_FrameDescriptorSetLayout, m_FrameDescriptorSet[i]);
		gfxDevice->WriteDescriptor(m_FrameInputLayout.bindings[0], m_FrameDescriptorSet[i], m_SceneBuffer[i]);
        gfxDevice->WriteDescriptor(m_FrameInputLayout.bindings[1], m_FrameDescriptorSet[i], m_SkyboxTexture);
        gfxDevice->WriteDescriptor(m_FrameInputLayout.bindings[2], m_FrameDescriptorSet[i], m_OffscreenPassResolvedColor);
        gfxDevice->WriteDescriptor(m_FrameInputLayout.bindings[3], m_FrameDescriptorSet[i], m_OffscreenResolvedDepth);
	}

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/OceanRendering/vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, m_TessellationControlShader, "../src/Samples/OceanRendering/tessellation_control.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_TessellationEvaluationShader, "../src/Samples/OceanRendering/tessellation_evaluation.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/OceanRendering/fragment.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_PostEffectsVertexShader, "../src/Samples/OceanRendering/post_effects_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_HDRPostProcessFragmentShader, "../src/Samples/OceanRendering/hdr_post_effects_fragment.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SkyboxVertexShader, "../src/Samples/OceanRendering/skybox_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SkyboxFragmentShader, "../src/Samples/OceanRendering/skybox_fragment.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_PostEffectsFragmentShader, "../src/Samples/OceanRendering/post_effects_fragment.glsl");

    Graphics::PipelineStateDescription psoDesc = {};

    psoDesc.Name = "Default PSO";
    psoDesc.vertexShader = &m_VertexShader;
    psoDesc.tessellationControlShader = &m_TessellationControlShader;
    psoDesc.tessellationEvaluationShader = &m_TessellationEvaluationShader;
    psoDesc.tessellationPatchControlPoints = 3;
    psoDesc.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    psoDesc.fragmentShader = &m_FragShader;
    psoDesc.psoInputLayout.push_back(m_FrameInputLayout);
    psoDesc.cullMode = VK_CULL_MODE_NONE;

    gfxDevice->CreatePipelineState(psoDesc, m_DefaultPSO, *m_OffscreenRenderTarget.get());

    psoDesc.Name = "Wireframe PSO";
    psoDesc.lineWidth = 2.0f;
    psoDesc.polygonMode = VK_POLYGON_MODE_LINE;

    gfxDevice->CreatePipelineState(psoDesc, m_WireframePSO, *m_OffscreenRenderTarget.get());

    Graphics::PipelineStateDescription hdrPostProcessDesc = {};
    hdrPostProcessDesc.Name = "HDR Post Effects";
    hdrPostProcessDesc.vertexShader = &m_PostEffectsVertexShader;
    hdrPostProcessDesc.fragmentShader = &m_HDRPostProcessFragmentShader;
    hdrPostProcessDesc.noVertex = true;
    hdrPostProcessDesc.cullMode = VK_CULL_MODE_NONE;
    hdrPostProcessDesc.psoInputLayout.push_back(m_FrameInputLayout);

    hdrPostProcessDesc.colorBlendingEnable = true;
    hdrPostProcessDesc.colorBlendingDesc.blendEnable = VK_TRUE;
    hdrPostProcessDesc.colorBlendingDesc.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    hdrPostProcessDesc.colorBlendingDesc.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    hdrPostProcessDesc.colorBlendingDesc.colorBlendOp = VK_BLEND_OP_ADD;

    gfxDevice->CreatePipelineState(hdrPostProcessDesc, m_HDRPostProcessPSO, *m_HDRPostProcessRenderTarget.get());
    
    Graphics::PipelineStateDescription skyboxDesc = {};
    skyboxDesc.Name = "Skybox";
    skyboxDesc.vertexShader = &m_SkyboxVertexShader;
    skyboxDesc.fragmentShader = &m_SkyboxFragmentShader;
    skyboxDesc.noVertex = true;
    skyboxDesc.psoInputLayout.push_back(m_FrameInputLayout);

    gfxDevice->CreatePipelineState(skyboxDesc, m_SkyboxPSO, *m_OffscreenRenderTarget.get());

    Graphics::PipelineStateDescription postEffectsDesc = {};
    postEffectsDesc.Name = "Post Effects PSO";
    postEffectsDesc.vertexShader = &m_PostEffectsVertexShader;
    postEffectsDesc.fragmentShader = &m_PostEffectsFragmentShader;
    postEffectsDesc.noVertex = true;
    postEffectsDesc.cullMode = VK_CULL_MODE_NONE;
    postEffectsDesc.attachmentCount = 1;
    postEffectsDesc.psoInputLayout.push_back(m_FrameInputLayout);

    gfxDevice->CreatePipelineState(postEffectsDesc, m_PostEffectsPSO, *gfxDevice->GetSwapChain().RenderTarget.get());
}

void OceanRendering::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget.reset();
    m_HDRPostProcessRenderTarget.reset();

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

    gfxDevice->DestroyImage(m_SkyboxTexture);
    gfxDevice->DestroyShader(m_SkyboxVertexShader);
    gfxDevice->DestroyShader(m_SkyboxFragmentShader);
    gfxDevice->DestroyPipeline(m_SkyboxPSO);

    gfxDevice->DestroyShader(m_PostEffectsVertexShader);
    gfxDevice->DestroyShader(m_PostEffectsFragmentShader);
    gfxDevice->DestroyPipeline(m_PostEffectsPSO);

    gfxDevice->DestroyShader(m_HDRPostProcessFragmentShader);
    gfxDevice->DestroyPipeline(m_HDRPostProcessPSO);

    gfxDevice->DestroyImage(m_OffscreenPassColor);
    gfxDevice->DestroyImage(m_OffscreenDepth);
    gfxDevice->DestroyImage(m_OffscreenResolvedDepth);
    gfxDevice->DestroyImage(m_OffscreenPassResolvedColor);
}

void OceanRendering::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("OceanRendering::Update");

	m_Camera.OnUpdate(deltaT, input);

    m_WaterModel->OnUpdate(deltaT);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    glm::vec2 screenSpaceLightPos = CalculateScreenSpaceLightPos(m_Camera.ProjectionMatrix, m_Camera.ViewMatrix, SampleSceneData.LightPosition);

    m_WaterModelMatrix = m_WaterModel->GetModelMatrix();

    SampleSceneData.Sun.x = screenSpaceLightPos.x;
    SampleSceneData.Sun.y = screenSpaceLightPos.y;
    SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
    SampleSceneData.ViewerPosition  = glm::vec4(m_Camera.Position, 1.0f);
    SampleSceneData.Time            = constantT;
    SampleSceneData.ImageWidth      = static_cast<float>(m_ScreenWidth);
    SampleSceneData.ImageHeight     = static_cast<float>(m_ScreenHeight);
    SampleSceneData.Flags           = (m_GenerateNormalPerFragment << 7
                                        | m_RandomWaveDirectionEnabled << 6 
                                        | m_ReflectionEnabled << 5
                                        | m_TessellationEnabled << 4
                                        | m_FractalBrownianMotionDomainWarpingEnabled << 3
                                        | m_DebugRenderWorldSpacePos << 2
                                        | m_CircularWavesEnabled << 1
                                        | m_DebugRenderNormals);

    if (!m_GPUCullingFreeze) {
        const glm::mat4 viewProj = m_Camera.ProjectionMatrix * m_Camera.ViewMatrix;

        const glm::vec4 r0 = glm::vec4(viewProj[0][0], viewProj[1][0], viewProj[2][0], viewProj[3][0]);
        const glm::vec4 r1 = glm::vec4(viewProj[0][1], viewProj[1][1], viewProj[2][1], viewProj[3][1]);
        const glm::vec4 r2 = glm::vec4(viewProj[0][2], viewProj[1][2], viewProj[2][2], viewProj[3][2]);
        const glm::vec4 r3 = glm::vec4(viewProj[0][3], viewProj[1][3], viewProj[2][3], viewProj[3][3]);

        // Gribb-Hartmann method to create frustum planes from viewProj matrix
        // is fater than geometric frustum construction since with heavy 
        // trigonometric functions (e.g., cos, tan).
        std::array<glm::vec4, 6> cameraFrustumPlanes = {};

        // After the vertex is multiplied by the view matrix, below can be concoluded
        // -w < x < w
        // -w < x                       -> x' is the inside halfspace of the left clipping plane
        // (v . row3) < (v . row0)
        // 0 < (v . row3) + (v . row0)
        // 0 < v . (row3 + row0)
        cameraFrustumPlanes[0] = glm::normalize(r3 + r0);   // left plane
        cameraFrustumPlanes[1] = glm::normalize(r3 - r0);   // right plane
        cameraFrustumPlanes[2] = glm::normalize(r3 + r1);   // bottom plane
        cameraFrustumPlanes[3] = glm::normalize(r3 - r1);   // top plane
        cameraFrustumPlanes[4] = glm::normalize(r2);        // near plane
        cameraFrustumPlanes[5] = glm::normalize(r3 - r2);   // far plane

        SampleSceneData.LocalSpaceCameraFrustumPlanes[0] = cameraFrustumPlanes[0] * m_WaterModelMatrix;
        SampleSceneData.LocalSpaceCameraFrustumPlanes[1] = cameraFrustumPlanes[1] * m_WaterModelMatrix;
        SampleSceneData.LocalSpaceCameraFrustumPlanes[2] = cameraFrustumPlanes[2] * m_WaterModelMatrix;
        SampleSceneData.LocalSpaceCameraFrustumPlanes[3] = cameraFrustumPlanes[3] * m_WaterModelMatrix;
        SampleSceneData.LocalSpaceCameraFrustumPlanes[4] = cameraFrustumPlanes[4] * m_WaterModelMatrix;
        SampleSceneData.LocalSpaceCameraFrustumPlanes[5] = cameraFrustumPlanes[5] * m_WaterModelMatrix;
       
        SampleSceneData.LocalSpaceCameraFrustumPlanes[0] /= glm::length(glm::vec3(SampleSceneData.LocalSpaceCameraFrustumPlanes[0]));
        SampleSceneData.LocalSpaceCameraFrustumPlanes[1] /= glm::length(glm::vec3(SampleSceneData.LocalSpaceCameraFrustumPlanes[1]));
        SampleSceneData.LocalSpaceCameraFrustumPlanes[2] /= glm::length(glm::vec3(SampleSceneData.LocalSpaceCameraFrustumPlanes[2]));
        SampleSceneData.LocalSpaceCameraFrustumPlanes[3] /= glm::length(glm::vec3(SampleSceneData.LocalSpaceCameraFrustumPlanes[3]));
        SampleSceneData.LocalSpaceCameraFrustumPlanes[4] /= glm::length(glm::vec3(SampleSceneData.LocalSpaceCameraFrustumPlanes[4]));
        SampleSceneData.LocalSpaceCameraFrustumPlanes[5] /= glm::length(glm::vec3(SampleSceneData.LocalSpaceCameraFrustumPlanes[5]));
    }

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], 0, &SampleSceneData, sizeof(SceneData));
}

void OceanRendering::RenderModel(
        const VkCommandBuffer& commandBuffer, 
        const uint32_t currentFrame, 
        const std::shared_ptr<Assets::Model>& model, 
        const Graphics::PipelineState& pipeline,
        const PushConstants& pushContants) const {

    SCOPED_PROFILER_US("OceanRendering::RenderModel");

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    VkDeviceSize offsets[] = { sizeof(uint32_t) * model->TotalIndices };

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model->DataBuffer.Handle, offsets);
    vkCmdBindIndexBuffer(commandBuffer, model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(commandBuffer, pipeline.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &FramePushConstants);

    for (const auto& mesh: model->Meshes) { 
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

    gfxDevice->BindDescriptorSet(m_FrameDescriptorSet[currentFrame], commandBuffer, m_DefaultPSO.pipelineLayout, 0, 1);

    FramePushConstants.Color.g = glm::radians(m_SkyboxRotation);
    FramePushConstants.Model = m_WaterModelMatrix;

    if (m_RenderWireframe) {
        RenderModel(commandBuffer, currentFrame, m_WaterModel, m_WireframePSO, FramePushConstants);
    } else {
        RenderModel(commandBuffer, currentFrame, m_WaterModel, m_DefaultPSO, FramePushConstants);
    }

    if (m_RenderSkybox) {
        RenderSkybox(currentFrame, commandBuffer);
    }

	m_OffscreenRenderTarget->End(commandBuffer);

   m_HDRPostProcessRenderTarget->Begin(commandBuffer);

    RenderPostEffects(currentFrame, commandBuffer, &m_HDRPostProcessPSO);

    m_HDRPostProcessRenderTarget->End(commandBuffer);

    gfxDevice->GetSwapChain().RenderTarget->Begin(commandBuffer);
    RenderPostEffects(currentFrame, commandBuffer, &m_PostEffectsPSO);
}

void OceanRendering::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

    if (ImGui::TreeNode("Skybox Settings")) {
        ImGui::DragFloat("Cube Size", &m_SkyboxCubeSize, 1.0f, 0.0f, 2000.0f);
        ImGui::DragFloat("Rotation (Y)", &m_SkyboxRotation, 1.0f, -360.0f, 360.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Pipeline Settings")) {
        ImGui::DragFloat("Fog Density",                     &SampleSceneData.FogDensity, 0.001f, -10.0f, 10.0f);
        ImGui::DragFloat("Fog Height Falloff",              &SampleSceneData.FogHeightFalloff, 0.001f, -10.0f, 10.0f);
        ImGui::Checkbox("Render Wireframe",				    &m_RenderWireframe);
        ImGui::Checkbox("Render Skybox",                    &m_RenderSkybox);
        ImGui::Checkbox("Tessellation Enabled",             &m_TessellationEnabled);
        ImGui::Checkbox("Reflection Enabled",               &m_ReflectionEnabled);
        ImGui::Checkbox("Generate normal per fragment",     &m_GenerateNormalPerFragment);
        ImGui::Checkbox("Debug - Render World Space Pos",   &m_DebugRenderWorldSpacePos);
        ImGui::Checkbox("Debug - Render Normals",           &m_DebugRenderNormals);
        ImGui::Checkbox("GPU Culling Freeze",               &m_GPUCullingFreeze);

        if (m_TessellationEnabled) {
            ImGui::DragFloat("Tessellation Min Threshold",  &SampleSceneData.TessellationMinThreshold, 1.0f, 0.0f, 500.0f);
            ImGui::DragFloat("Tessellation Max Threshold",  &SampleSceneData.TessellationMaxThreshold, 1.0f, 0.0f, 500.0f);
            ImGui::DragFloat("Tessellation Level Min",      &SampleSceneData.TessellationLevelMin, 1.0f, 1.0f, 64.0f);
            ImGui::DragFloat("Tessellation Level Max",      &SampleSceneData.TessellationLevelMax, 1.0f, 1.0f, 64.0f);
            ImGui::DragFloat("Tessellation Step",           &SampleSceneData.TessellationStep, 0.1f, 0.0f, 50.0f);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Water Material Settings")) {
        ImGui::ColorPicker3("Water Color",              (float*)&SampleSceneData.WaterColor);
        ImGui::DragFloat("Water Ambient Color Strength",&SampleSceneData.WaterColor.w, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Water Shininess",             &SampleSceneData.WaterShininess, 1.0f, 0.0f, 3000.0f);
        ImGui::DragFloat("Water Reflection Strength" ,  &SampleSceneData.ReflectionStrength, 0.01f, -10.0f, 10.0f);
        ImGui::DragFloat("Specular Displacement",       &SampleSceneData.SpecularDisplacement, 0.0001f, -2.0f, 2.0f);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Light Settings")) {
        ImGui::DragFloat3("Light Direction",(float*)&SampleSceneData.LightPosition, 1.0f, -4000.0f, 4000.0f);
        ImGui::DragFloat("Light Strength",  &SampleSceneData.LightPosition.w, 0.02f, 0.0f, 200.0f);
        ImGui::ColorPicker3("Light Color",  (float*)&SampleSceneData.LightColor);
        ImGui::DragFloat("Light Specular",	&SampleSceneData.LightColor.w, 1.0f, 0.0f, 1000.0f);
        ImGui::DragFloat("Sun Radius",	    &SampleSceneData.Sun.z, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Sun Strength",	&SampleSceneData.Sun.w, 0.01f, 0.0f, 5.0f);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Wave Settings")) {
        ImGui::DragInt("Wave Count",                                        &SampleSceneData.WaveCount, 1.0f, 0, 200);
        ImGui::Checkbox("Circular Waves Enabled",                           &m_CircularWavesEnabled);
        ImGui::Checkbox("Random Wave Direction Enabled",                    &m_RandomWaveDirectionEnabled);
        ImGui::Checkbox("Fractal Brownian Motion Domain Warping Enabled",   &m_FractalBrownianMotionDomainWarpingEnabled);
        ImGui::DragFloat("Water Depth",                                     &SampleSceneData.WaterDepth, 0.01f, 0.0f, 50.0f);
        ImGui::DragFloat("Sine FBM Wave Amplitude",                         &SampleSceneData.SineFBMAmplitude, 0.001f, 0.0f, 100.0f);
        ImGui::DragFloat("Sine FBM Wave Frequency",                         &SampleSceneData.SineFBMFrequency, 0.001f, 0.0f, 100.0f);
        ImGui::DragFloat("Sine FBM Wave Amplitude Multiplier",              &SampleSceneData.SineFBMAmplitudeMultiplier, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Sine FBM Wave Frequency Multiplier",              &SampleSceneData.SineFBMFrequencyMultiplier, 0.001f, 1.0f, 10.0f);
        ImGui::DragFloat("Drag Mult",                                       &SampleSceneData.DragMult, 1.0f, -100.0f, 100.0f);
        ImGui::DragFloat("Wind Angle",                                      &SampleSceneData.WindAngle, 0.01f, -90.0f, 90.0f);
        ImGui::DragFloat("Wind Speed",                                      &SampleSceneData.WindSpeed, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Temporal Phase Exponent",                         &SampleSceneData.TemporalPhaseExponent, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Height Multiplier",                               &SampleSceneData.HeightMultiplier, 0.01f, 0.0f, 10.0f);

        ImGui::TreePop();
    }

	ImGui::SeparatorText("Models Settings");
    m_WaterModel->OnUIRender();
}

void OceanRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);

    CreateDisplaySizeDependentResources(m_ScreenWidth, m_ScreenHeight);

	m_OffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_OffscreenRenderPassDescription);
    m_HDRPostProcessRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_HDRPostEffectsRenderPassDescription);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
   
    for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        gfxDevice->WriteDescriptor(m_FrameInputLayout.bindings[2], m_FrameDescriptorSet[i], m_OffscreenPassResolvedColor);
        gfxDevice->WriteDescriptor(m_FrameInputLayout.bindings[3], m_FrameDescriptorSet[i], m_OffscreenResolvedDepth);
	}
}

RUN_APPLICATION(OceanRendering);
