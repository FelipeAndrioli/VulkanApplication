#include <iostream>
#include <random>

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
constexpr float Lerp(float a, float b, float f) { return a + f * (b - a); }

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
        alignas(16) glm::vec4 ViewerPosition;
		alignas(16) glm::vec4 Light;
		alignas(16) glm::vec4 LightView;
        alignas(4) int Flags;
        alignas(4) int Extra_1;
        alignas(4) int Extra_2;
        alignas(4) int Extra_3;
	} SampleSceneUBOData;

	struct PushConstants {
		alignas(16) glm::mat4 Model;
        alignas(4) int MaterialIndex;
        alignas(4) float SpecularFactor = 0.0f;
	} SamplePushConstants;

    struct PostProcessUBO {
        alignas(16) glm::vec4 Extra[15];
        alignas(4) float Gamma = 2.2f;
        alignas(4) float Extra1 = 0.0f;
        alignas(4) float Extra2 = 0.0f;
        alignas(4) float Extra3 = 0.0f;
    } PostProcessUBOData;

    struct SSAOUBO {
        alignas(16) glm::mat4 Projection;

        // this is wasting one float per kernel sample due to alignment, 256 bytes wasted :)
        alignas(16) glm::vec4 SSAOKernel[64];
        alignas(4) int KernelSize = 64;
        alignas(4) int ScreenWidth;
        alignas(4) int ScreenHeight;
        alignas(4) int Flags;
        alignas(4) float Radius = 0.5f;
        alignas(4) float Bias = 0.025f;
    } SSAOUBOData;

	const glm::vec3 InitialCameraPosition = glm::vec3(-10.0f, -3.5f, -0.2f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= 1.0f;
	const float InitialCameraPitch	= -10.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;
    uint32_t m_SampleCount = 1;

    bool m_FinalImageFirstFrame = true;
    bool m_RenderPostEffects = true;
    bool m_RenderSSAO = true;
    bool m_BlurSSAOEnabled = true;
    bool m_SSAODebugViewEnabled = false;

	std::array<std::shared_ptr<Assets::Model>, TOTAL_MODELS> m_Models;

	size_t TotalModels = 0;

    glm::vec4 m_Light = glm::vec4(0.1f, 1.0f, 0.0f, 0.1f);

    // Forward Pass
	std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_ForwardPassOffscreenRenderTarget;
    Graphics::RenderPassDescription m_ForwardPassDescription = {};
    Graphics::GPUImage m_ForwardColorBuffer;
    Graphics::GPUImage m_ForwardDepthBuffer;
    Graphics::GPUImage m_ForwardResolveBuffer;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::Buffer m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT] = {};

	Graphics::PipelineState m_PSO = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_Set = { VK_NULL_HANDLE };
    // Forward Pass

    std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_PostEffectsRenderTarget;
    Graphics::RenderPassDescription m_PostEffectsPassDescription = {}; 
    Graphics::GPUImage m_PostEffectsBuffer;
    Graphics::Shader m_PostEffectsVertexShader = {};
    Graphics::Shader m_PostEffectsFragmentShader = {};
    Graphics::PipelineState m_PostEffectsPSO = {};
    Graphics::Buffer m_PostEffectsUBO = {};
    Graphics::InputLayout m_PostEffectsInputLayout = {};

	VkDescriptorSetLayout m_PostEffectsSetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_PostEffectsForwardPassSet = { VK_NULL_HANDLE };
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_PostEffectsSSAOPassSet = { VK_NULL_HANDLE };

    // SSAO Geometry Pass
    std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_GeometryPassRenderTarget;
    Graphics::RenderPassDescription m_GeometryPassDescription = {};
    Graphics::GPUImage m_GeometryPositionBuffer = {};
    Graphics::GPUImage m_GeometryNormalBuffer = {};
    Graphics::GPUImage m_GeometryAlbedoBuffer = {};
    Graphics::GPUImage m_GeometryDepthBuffer = {};

    Graphics::Shader m_GeometryPassVertexShader = {};
    Graphics::Shader m_GeometryPassFragmentShader = {};
    Graphics::PipelineState m_GeometryPassPSO = {};
    Graphics::Buffer m_GeometryPassUBO = {};
    Graphics::InputLayout m_GeometryPassInputLayout = {};

    VkDescriptorSetLayout m_GeometryPassSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_GeometryPassSet[Graphics::FRAMES_IN_FLIGHT];

    // SSAO Geometry Pass

    // SSAO Pass
    std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_SSAORenderTarget;
    Graphics::RenderPassDescription m_SSAOPassDescription = {};
    Graphics::GPUImage m_SSAOBuffer = {};
    Graphics::Shader m_SSAOVertexShader = {};
    Graphics::Shader m_SSAOFragmentShader = {};
    Graphics::PipelineState m_SSAOPassPSO = {};
    Graphics::GPUBuffer m_SSAOUBO = {};
    Graphics::InputLayout m_SSAOInputLayout = {};

    VkDescriptorSetLayout m_SSAOSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_SSAOSet[Graphics::FRAMES_IN_FLIGHT];
  
    Graphics::GPUImage m_SSAONoise = {};
    // SSAO Pass
   
    // SSAO Lighting Pass
    std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_LightCompositionRenderTarget;
    Graphics::RenderPassDescription m_LightCompositionPassDescription = {};
    Graphics::GPUImage m_LightCompositionBuffer = {};
    Graphics::Shader m_LightCompositionVertexShader = {};
    Graphics::Shader m_LightCompositionFragmentShader = {};
    Graphics::PipelineState m_LightCompositionPSO = {};
    Graphics::GPUBuffer m_LightCompositionUBO = {};
    Graphics::InputLayout m_LightCompositionInputLayout = {};

    VkDescriptorSetLayout m_LightCompositionSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_LightCompositionSetWithoutSSAOBlur[Graphics::FRAMES_IN_FLIGHT];
    VkDescriptorSet m_LightCompositionSetWithSSAOBlur[Graphics::FRAMES_IN_FLIGHT];

    // SSAO Lighting Pass

    // SSAO Blur Pass
    std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_SSAOBlurRenderTarget;
    Graphics::RenderPassDescription m_SSAOBlurRenderPassDescription = {};
    Graphics::GPUImage m_SSAOBlurBuffer = {};
    Graphics::Shader m_SSAOBlurVertexShader = {};
    Graphics::Shader m_SSAOBlurFragmentShader = {};
    Graphics::PipelineState m_SSAOBlurPSO = {};
    Graphics::InputLayout m_SSAOBlurInputLayout = {};

    VkDescriptorSetLayout m_SSAOBlurSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_SSAOBlurSet[Graphics::FRAMES_IN_FLIGHT];
    // SSAO Blur Pass
private:
    void InititalizeDisplaySizeDependentResources(uint32_t width, uint32_t height);

    void InitializeSSAO();
    void GenerateSSAOKernel(glm::vec4 kernel[64]);
    Graphics::GPUImage GenerateSSAONoise();

    void FullScreenPass(const VkCommandBuffer& commandBuffer, VkDescriptorSet& set, const VkPipelineLayout& pipelineLayout, const VkPipeline& pipeline, Graphics::IRenderTarget* renderTarget);

    void RenderNormalScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void ForwardLightCompositionPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void ForwardPostEffectsPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);

    void RenderSSAO(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void SSAOGeometryPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void SSAOPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void SSAOBlurPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void SSAOLightCompositionPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
    void SSAOPostEffectsPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
};

void AmbientOcclusion::StartUp() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
    ResourceManager* rm = ResourceManager::Get();

	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

    m_Light.w = 0.5f;
    
    m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);
    m_Camera.MovementSpeed = 0.001f;
    m_Camera.Sensitivity = 0.01f;

    m_Models[TotalModels] = ModelLoader::LoadModel("C:/Users/felip/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj");
    m_Models[TotalModels]->Transformations.scaleHandler = 0.008f;
    m_Models[TotalModels]->ModelIndex = TotalModels;

    TotalModels++;

    SSAOUBOData.Flags = 1; 

    for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
        m_SceneBuffer[i] = gfxDevice->CreateBuffer(sizeof(SceneUBOData));
    }

    m_PostEffectsUBO = gfxDevice->CreateBuffer(sizeof(PostProcessUBO));

    InititalizeDisplaySizeDependentResources(m_ScreenWidth, m_ScreenHeight);

    InitializeSSAO();

    m_ForwardPassOffscreenRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth,
        m_ScreenHeight,
        gfxDevice->GetMsaaSamples(),
        m_ForwardPassDescription);

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/AmbientOcclusion/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/AmbientOcclusion/fragment.glsl");
 
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

	gfxDevice->CreatePipelineState(desc, m_PSO, *m_ForwardPassOffscreenRenderTarget.get());
	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, inputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[0], m_Set[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[1], m_Set[i], rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(inputLayout.bindings[2], m_Set[i], rm->GetTextures());
	}

    m_PostEffectsRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth, 
        m_ScreenHeight,
        m_SampleCount,
        m_PostEffectsPassDescription);

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_PostEffectsVertexShader, "../src/Samples/AmbientOcclusion/quad_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_PostEffectsFragmentShader, "../src/Samples/AmbientOcclusion/post_process_fragment.glsl");

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
		gfxDevice->CreateDescriptorSet(m_PostEffectsSetLayout, m_PostEffectsForwardPassSet[i]);
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[0], m_PostEffectsForwardPassSet[i], m_PostEffectsUBO);
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[1], m_PostEffectsForwardPassSet[i], m_ForwardResolveBuffer);

        gfxDevice->CreateDescriptorSet(m_PostEffectsSetLayout, m_PostEffectsSSAOPassSet[i]);
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[0], m_PostEffectsSSAOPassSet[i], m_PostEffectsUBO);
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[1], m_PostEffectsSSAOPassSet[i], m_LightCompositionBuffer);
    }
}

void AmbientOcclusion::InititalizeDisplaySizeDependentResources(uint32_t width, uint32_t height) {

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    gfxDevice->DestroyImage(m_ForwardColorBuffer);
    gfxDevice->DestroyImage(m_ForwardDepthBuffer);
    gfxDevice->DestroyImage(m_ForwardResolveBuffer);

    gfxDevice->CreateRenderTarget(m_ForwardColorBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_ScreenWidth,
        m_ScreenHeight,
        gfxDevice->GetMsaaSamples());

    gfxDevice->CreateRenderTarget(m_ForwardResolveBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    gfxDevice->CreateDepthBuffer(m_ForwardDepthBuffer,
        gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
        m_ScreenWidth,
        m_ScreenHeight,
        gfxDevice->GetMsaaSamples());

    m_ForwardPassDescription.Attachments.clear();

    m_ForwardPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
            m_ForwardColorBuffer,
            Graphics::Format::R16G16B16A16_FLOAT,
            gfxDevice->GetMsaaSamples(),
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::SHADER_RESOURCE));

    m_ForwardPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::DepthStencil(
            m_ForwardDepthBuffer,
            gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
            gfxDevice->GetMsaaSamples(),
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::DEPTHSTENCIL,
            Graphics::ResourceState::DEPTHSTENCIL));

    m_ForwardPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::Resolve(
            m_ForwardResolveBuffer,
            Graphics::Format::R16G16B16A16_FLOAT,
            1,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::SHADER_RESOURCE));

    gfxDevice->DestroyImage(m_GeometryPositionBuffer);
    gfxDevice->DestroyImage(m_GeometryNormalBuffer);
    gfxDevice->DestroyImage(m_GeometryAlbedoBuffer);
    gfxDevice->DestroyImage(m_GeometryDepthBuffer);

    gfxDevice->CreateRenderTarget(m_GeometryPositionBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    gfxDevice->CreateRenderTarget(m_GeometryNormalBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    gfxDevice->CreateRenderTarget(m_GeometryAlbedoBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    gfxDevice->CreateDepthBuffer(m_GeometryDepthBuffer,
        gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    m_GeometryPassDescription.Attachments.clear();

    m_GeometryPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
        m_GeometryPositionBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_SampleCount,
        Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
        Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
        Graphics::ResourceState::UNDEFINED,
        Graphics::ResourceState::RENDERTARGET,
        Graphics::ResourceState::SHADER_RESOURCE));

    m_GeometryPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
        m_GeometryNormalBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_SampleCount,
        Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
        Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
        Graphics::ResourceState::UNDEFINED,
        Graphics::ResourceState::RENDERTARGET,
        Graphics::ResourceState::SHADER_RESOURCE));

    m_GeometryPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
        m_GeometryAlbedoBuffer,
        Graphics::Format::R16G16B16A16_FLOAT,
        m_SampleCount,
        Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
        Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
        Graphics::ResourceState::UNDEFINED,
        Graphics::ResourceState::RENDERTARGET,
        Graphics::ResourceState::SHADER_RESOURCE));

    m_GeometryPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::DepthStencil(
        m_GeometryDepthBuffer,
        gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
        m_SampleCount,
        Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
        Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
        Graphics::ResourceState::UNDEFINED,
        Graphics::ResourceState::DEPTHSTENCIL,
        Graphics::ResourceState::DEPTHSTENCIL));

    gfxDevice->DestroyImage(m_SSAOBuffer);

    gfxDevice->CreateRenderTarget(m_SSAOBuffer,
        Graphics::Format::R16_FLOAT,
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    m_SSAOPassDescription.Attachments.clear();
    
    m_SSAOPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
            m_SSAOBuffer,
            Graphics::Format::R16_FLOAT,
            m_SampleCount,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::SHADER_RESOURCE));

    gfxDevice->DestroyImage(m_LightCompositionBuffer);

    gfxDevice->CreateRenderTarget(m_LightCompositionBuffer,
        gfxDevice->ConvertFormat(gfxDevice->GetSwapChain().ImageFormat),
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    m_LightCompositionPassDescription.Attachments.clear();

    m_LightCompositionPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
            m_LightCompositionBuffer,
            gfxDevice->ConvertFormat(gfxDevice->GetSwapChain().ImageFormat),
            m_SampleCount,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::SHADER_RESOURCE));

    gfxDevice->DestroyImage(m_SSAOBlurBuffer);

    gfxDevice->CreateRenderTarget(m_SSAOBlurBuffer,
        Graphics::Format::R16_FLOAT,
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    m_SSAOBlurRenderPassDescription.Attachments.clear();

    m_SSAOBlurRenderPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
            m_SSAOBlurBuffer,
            Graphics::Format::R16_FLOAT,
            m_SampleCount,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::SHADER_RESOURCE));

    gfxDevice->DestroyImage(m_PostEffectsBuffer);

    gfxDevice->CreateRenderTarget(
        m_PostEffectsBuffer,
        gfxDevice->ConvertFormat(gfxDevice->GetSwapChain().ImageFormat),
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount);

    m_PostEffectsPassDescription.Attachments.clear();

    m_PostEffectsPassDescription.Attachments.push_back(
        Graphics::RenderPassAttachment::RenderTarget(
            m_PostEffectsBuffer,
            gfxDevice->ConvertFormat(gfxDevice->GetSwapChain().ImageFormat),
            m_SampleCount,
            Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
            Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
            Graphics::ResourceState::UNDEFINED,
            Graphics::ResourceState::RENDERTARGET,
            Graphics::ResourceState::SHADER_RESOURCE));
}

void AmbientOcclusion::InitializeSSAO() {

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
   
    m_GeometryPassRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount,
        m_GeometryPassDescription);

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_GeometryPassVertexShader, "../src/Samples/AmbientOcclusion/geometry_pass_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_GeometryPassFragmentShader, "../src/Samples/AmbientOcclusion/geometry_pass_fragment.glsl");

    ResourceManager* rm = ResourceManager::Get();

    m_GeometryPassUBO = gfxDevice->CreateBuffer(sizeof(SceneUBOData));

    m_GeometryPassInputLayout = {
        .pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
        },
        .bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },			
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(rm->GetTextures().size()), VK_SHADER_STAGE_FRAGMENT_BIT },			
        }
    };

    Graphics::PipelineStateDescription GeometryPassPSODesc = {};
    GeometryPassPSODesc.Name = "SSAO Geometry Pass";
    GeometryPassPSODesc.vertexShader = &m_GeometryPassVertexShader;
    GeometryPassPSODesc.fragmentShader = &m_GeometryPassFragmentShader;
    GeometryPassPSODesc.attachmentCount = 3; // Position, Normal, AlbedoSpec (more like a color attachment count)
    GeometryPassPSODesc.cullMode = VK_CULL_MODE_BACK_BIT;
    GeometryPassPSODesc.psoInputLayout.push_back(m_GeometryPassInputLayout);

    gfxDevice->CreatePipelineState(GeometryPassPSODesc, m_GeometryPassPSO, *m_GeometryPassRenderTarget.get());
    gfxDevice->CreateDescriptorSetLayout(m_GeometryPassSetLayout, m_GeometryPassInputLayout.bindings);

    for (uint32_t frameIndex = 0; frameIndex < Graphics::FRAMES_IN_FLIGHT; frameIndex++) {
        gfxDevice->CreateDescriptorSet(m_GeometryPassSetLayout, m_GeometryPassSet[frameIndex]); 
        gfxDevice->WriteDescriptor(m_GeometryPassInputLayout.bindings[0], m_GeometryPassSet[frameIndex], m_SceneBuffer[frameIndex]);
        gfxDevice->WriteDescriptor(m_GeometryPassInputLayout.bindings[1], m_GeometryPassSet[frameIndex], rm->GetMaterialBuffer());
        gfxDevice->WriteDescriptor(m_GeometryPassInputLayout.bindings[2], m_GeometryPassSet[frameIndex], rm->GetTextures());
    }

    GenerateSSAOKernel(SSAOUBOData.SSAOKernel);
    m_SSAONoise = GenerateSSAONoise();

    m_SSAORenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount,
        m_SSAOPassDescription);

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SSAOVertexShader, "../src/Samples/AmbientOcclusion/quad_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SSAOFragmentShader, "../src/Samples/AmbientOcclusion/ssao_fragment.glsl");

    m_SSAOUBO = gfxDevice->CreateStorageBuffer(sizeof(SSAOUBO));

    m_SSAOInputLayout = {
        .pushConstants = {},
        .bindings = {
            { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // Position
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // Normal
            { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // Albedo Spec 
            { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // Noise
        }
    };

    Graphics::PipelineStateDescription SSAOPSODesc = {};
    SSAOPSODesc.Name = "SSAO Pass";
    SSAOPSODesc.noVertex = true;
    SSAOPSODesc.cullMode = VK_CULL_MODE_NONE;
    SSAOPSODesc.vertexShader = &m_SSAOVertexShader;
    SSAOPSODesc.fragmentShader = &m_SSAOFragmentShader;
    SSAOPSODesc.attachmentCount = 1;
    SSAOPSODesc.psoInputLayout.push_back(m_SSAOInputLayout);

    gfxDevice->CreatePipelineState(SSAOPSODesc, m_SSAOPassPSO, *m_SSAORenderTarget.get());
    gfxDevice->CreateDescriptorSetLayout(m_SSAOSetLayout, m_SSAOInputLayout.bindings);

    for (uint32_t frameIndex = 0; frameIndex < Graphics::FRAMES_IN_FLIGHT; frameIndex++) {
        gfxDevice->CreateDescriptorSet(m_SSAOSetLayout, m_SSAOSet[frameIndex]);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[0], m_SSAOSet[frameIndex], m_SSAOUBO);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[1], m_SSAOSet[frameIndex], m_GeometryPositionBuffer);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[2], m_SSAOSet[frameIndex], m_GeometryNormalBuffer);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[3], m_SSAOSet[frameIndex], m_GeometryAlbedoBuffer);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[4], m_SSAOSet[frameIndex], m_SSAONoise);
    }

    m_SSAOBlurRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount,
        m_SSAOBlurRenderPassDescription);

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SSAOBlurVertexShader, "../src/Samples/AmbientOcclusion/quad_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SSAOBlurFragmentShader, "../src/Samples/AmbientOcclusion/ssao_blur_fragment.glsl");

    m_SSAOBlurInputLayout = {
        .pushConstants = {},
        .bindings = {
            { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }  // SSAO
        }
    };

    Graphics::PipelineStateDescription SSAOBlurPSODesc = {};
    SSAOBlurPSODesc.Name = "SSAO Blur";
    SSAOBlurPSODesc.noVertex = true;
    SSAOBlurPSODesc.cullMode = VK_CULL_MODE_NONE;
    SSAOBlurPSODesc.vertexShader = &m_SSAOBlurVertexShader;
    SSAOBlurPSODesc.fragmentShader = &m_SSAOBlurFragmentShader;
    SSAOBlurPSODesc.attachmentCount = 1;
    SSAOBlurPSODesc.psoInputLayout.push_back(m_SSAOBlurInputLayout);

    gfxDevice->CreatePipelineState(SSAOBlurPSODesc, m_SSAOBlurPSO, *m_SSAOBlurRenderTarget.get());
    gfxDevice->CreateDescriptorSetLayout(m_SSAOBlurSetLayout, m_SSAOBlurInputLayout.bindings);

    for (uint32_t frameIndex = 0; frameIndex < Graphics::FRAMES_IN_FLIGHT; frameIndex++) {
        gfxDevice->CreateDescriptorSet(m_SSAOBlurSetLayout, m_SSAOBlurSet[frameIndex]);
        gfxDevice->WriteDescriptor(m_SSAOBlurInputLayout.bindings[0], m_SSAOBlurSet[frameIndex], m_SSAOBuffer);
    }

    m_LightCompositionRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
        m_ScreenWidth,
        m_ScreenHeight,
        m_SampleCount,
        m_LightCompositionPassDescription);

    gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightCompositionVertexShader, "../src/Samples/AmbientOcclusion/quad_vertex.glsl");
    gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightCompositionFragmentShader, "../src/Samples/AmbientOcclusion/light_composition_fragment.glsl");

    m_LightCompositionInputLayout = {
        .pushConstants = {
            { VK_SHADER_STAGE_ALL, 0, sizeof(float) }
        },
        .bindings = {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // Position
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // Normal
            { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // Albedo Spec 
            { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // SSAO 
            { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },      // SSAO Blur
        }
    };

    Graphics::PipelineStateDescription LightCompositionPSODesc = {};
    LightCompositionPSODesc.Name = "Light Composition PSO";
    LightCompositionPSODesc.noVertex = true;
    LightCompositionPSODesc.cullMode = VK_CULL_MODE_NONE;
    LightCompositionPSODesc.vertexShader = &m_LightCompositionVertexShader;
    LightCompositionPSODesc.fragmentShader = &m_LightCompositionFragmentShader;
    LightCompositionPSODesc.attachmentCount = 1;
    LightCompositionPSODesc.psoInputLayout.push_back(m_LightCompositionInputLayout);

    gfxDevice->CreatePipelineState(LightCompositionPSODesc, m_LightCompositionPSO, *m_LightCompositionRenderTarget.get());
    gfxDevice->CreateDescriptorSetLayout(m_LightCompositionSetLayout, m_LightCompositionInputLayout.bindings);

    for (uint32_t frameIndex = 0; frameIndex < Graphics::FRAMES_IN_FLIGHT; frameIndex++) {
        gfxDevice->CreateDescriptorSet(m_LightCompositionSetLayout, m_LightCompositionSetWithoutSSAOBlur[frameIndex]);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[0], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_SceneBuffer[frameIndex]);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[1], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_GeometryPositionBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[2], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_GeometryNormalBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[3], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_GeometryAlbedoBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[4], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_SSAOBuffer);

        gfxDevice->CreateDescriptorSet(m_LightCompositionSetLayout, m_LightCompositionSetWithSSAOBlur[frameIndex]);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[0], m_LightCompositionSetWithSSAOBlur[frameIndex], m_SceneBuffer[frameIndex]);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[1], m_LightCompositionSetWithSSAOBlur[frameIndex], m_GeometryPositionBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[2], m_LightCompositionSetWithSSAOBlur[frameIndex], m_GeometryNormalBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[3], m_LightCompositionSetWithSSAOBlur[frameIndex], m_GeometryAlbedoBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[4], m_LightCompositionSetWithSSAOBlur[frameIndex], m_SSAOBlurBuffer);
    }
}

void AmbientOcclusion::GenerateSSAOKernel(glm::vec4 kernel[64]) {

    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f); // random floats between [0.0, 1.0]
    std::default_random_engine generator;

    for (uint32_t i = 0; i < 64; i++) {

        // vary x and y direction in tangent space between -1.0 and 1.0, while
        // z samples will vary between 0.0 and 1.0 (if we varied z direction
        // between -1.0 and 1.0 as well we'd have a sphere sample kernel).
        glm::vec4 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator),
            0.0f);

        sample = glm::normalize(sample);

        // randomize samples displacement
        sample *= randomFloats(generator);

        // displace the samples to be closer to the origin
        float scale = (float)i / 64.0f;
        scale = Lerp(0.1f, 1.0f, scale * scale);

        sample *= scale;

        kernel[i] = sample;
    }
}

Graphics::GPUImage AmbientOcclusion::GenerateSSAONoise() {

    Graphics::GPUImage result = {};

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
   
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f); // random floats between [0.0, 1.0]
    std::default_random_engine generator;

    std::vector<glm::vec4> ssaoNoise;

    for (uint32_t i = 0; i < 16; i++) {
        glm::vec4 noise (
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f,
            0.0f);

        ssaoNoise.push_back(noise);
    }

    Graphics::ImageDescription desc = {
        .Width				= 4,
        .Height				= 4,
        .MipLevels			= 1,
        .LayerCount			= 1,
        .Format				= VK_FORMAT_R32G32B32A32_SFLOAT,
        .Tiling				= VK_IMAGE_TILING_OPTIMAL,
        .Usage				= static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
        .MemoryProperty		= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT,
        .ViewType			= VK_IMAGE_VIEW_TYPE_2D,
        .MsaaSamples		= VK_SAMPLE_COUNT_1_BIT,
        .ImageType			= VK_IMAGE_TYPE_2D,
        .AddressMode		= VK_SAMPLER_ADDRESS_MODE_REPEAT
    };

    gfxDevice->CreateImage(result, desc);
    gfxDevice->CreateImageView(result);
    gfxDevice->TransitionImageLayout(result, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    gfxDevice->UploadDataToImage(result, ssaoNoise.data(), (size_t)(desc.Width * desc.Height * 4 * sizeof(float)));
    gfxDevice->CreateImageSampler(result);
    gfxDevice->TransitionImageLayout(result, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return result;
}

void AmbientOcclusion::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_ForwardPassOffscreenRenderTarget.reset();
    
    gfxDevice->DestroyImage(m_ForwardColorBuffer);
    gfxDevice->DestroyImage(m_ForwardDepthBuffer);
    gfxDevice->DestroyImage(m_ForwardResolveBuffer);

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(m_PSO);

    m_PostEffectsRenderTarget.reset();

    gfxDevice->DestroyImage(m_PostEffectsBuffer);
    gfxDevice->DestroyShader(m_PostEffectsVertexShader);
    gfxDevice->DestroyShader(m_PostEffectsFragmentShader);
	gfxDevice->DestroyDescriptorSetLayout(m_PostEffectsSetLayout);
	gfxDevice->DestroyPipeline(m_PostEffectsPSO);

    m_GeometryPassRenderTarget.reset();

    gfxDevice->DestroyShader(m_GeometryPassVertexShader);
    gfxDevice->DestroyShader(m_GeometryPassFragmentShader);
    gfxDevice->DestroyDescriptorSetLayout(m_GeometryPassSetLayout);
    gfxDevice->DestroyPipeline(m_GeometryPassPSO);

    gfxDevice->DestroyImage(m_GeometryPositionBuffer);
    gfxDevice->DestroyImage(m_GeometryNormalBuffer);
    gfxDevice->DestroyImage(m_GeometryAlbedoBuffer);
    gfxDevice->DestroyImage(m_GeometryDepthBuffer);

    gfxDevice->DestroyDescriptorSetLayout(m_SSAOSetLayout);
    gfxDevice->DestroyPipeline(m_SSAOPassPSO);
    gfxDevice->DestroyShader(m_SSAOVertexShader);
    gfxDevice->DestroyShader(m_SSAOFragmentShader);
    gfxDevice->DestroyImage(m_SSAOBuffer);
    gfxDevice->DestroyImage(m_SSAONoise);
    gfxDevice->DestroyBuffer(m_SSAOUBO);

    m_LightCompositionRenderTarget.reset();

    gfxDevice->DestroyDescriptorSetLayout(m_LightCompositionSetLayout);
    gfxDevice->DestroyPipeline(m_LightCompositionPSO);
    gfxDevice->DestroyShader(m_LightCompositionVertexShader);
    gfxDevice->DestroyShader(m_LightCompositionFragmentShader);
    gfxDevice->DestroyImage(m_LightCompositionBuffer);

    m_SSAOBlurRenderTarget.reset();

    gfxDevice->DestroyDescriptorSetLayout(m_SSAOBlurSetLayout);
    gfxDevice->DestroyPipeline(m_SSAOBlurPSO);
    gfxDevice->DestroyShader(m_SSAOBlurVertexShader);
    gfxDevice->DestroyShader(m_SSAOBlurFragmentShader);
    gfxDevice->DestroyImage(m_SSAOBlurBuffer);
}

void AmbientOcclusion::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("AmbientOcclusion::Update");

	m_Camera.OnUpdate(deltaT, input);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	SampleSceneUBOData.Projection = m_Camera.ProjectionMatrix;
	SampleSceneUBOData.View	= m_Camera.ViewMatrix;
    SampleSceneUBOData.ViewerPosition = glm::vec4(m_Camera.Position, 1.0f);
	SampleSceneUBOData.Light = m_Light;
    SampleSceneUBOData.LightView = m_Camera.ViewMatrix * glm::vec4(m_Light.x, m_Light.y, m_Light.z, 1.0f);

    SampleSceneUBOData.Flags = (m_BlurSSAOEnabled << 1 | m_SSAODebugViewEnabled << 0);

    SSAOUBOData.Projection = SampleSceneUBOData.Projection;
    SSAOUBOData.ScreenWidth = m_ScreenWidth;
    SSAOUBOData.ScreenHeight = m_ScreenHeight;

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], &SampleSceneUBOData);
	gfxDevice->UpdateBuffer(m_PostEffectsUBO, &PostProcessUBOData);
    gfxDevice->UpdateBuffer(m_SSAOUBO, 0, &SSAOUBOData, sizeof(SSAOUBO));
}

void AmbientOcclusion::RenderSSAO(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::RenderSSAO");

    SSAOGeometryPass(currentFrame, commandBuffer);
    SSAOPass(currentFrame, commandBuffer);
    
    if (m_BlurSSAOEnabled) {
        SSAOBlurPass(currentFrame, commandBuffer);
    }

    SSAOLightCompositionPass(currentFrame, commandBuffer);

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    Graphics::GPUImage* lastImage = &m_LightCompositionBuffer;

    if (m_RenderPostEffects) {
        SSAOPostEffectsPass(currentFrame, commandBuffer);

        lastImage = &m_PostEffectsBuffer;
    } 

    if (m_FinalImageFirstFrame) {
        gfxDevice->TransitionImageLayout(*lastImage, Graphics::ResourceState::UNDEFINED, Graphics::ResourceState::COPY_SRC);
        m_FinalImageFirstFrame = false;
    } else {
        gfxDevice->TransitionImageLayout(*lastImage, Graphics::ResourceState::SHADER_RESOURCE, Graphics::ResourceState::COPY_SRC);
    }

    gfxDevice->GetSwapChain().RenderTarget->CopyColor(*lastImage);
}

void AmbientOcclusion::SSAOGeometryPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::SSAOGeometryPass");

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    m_GeometryPassRenderTarget->Begin(commandBuffer);

    gfxDevice->BindDescriptorSet(m_GeometryPassSet[currentFrame], commandBuffer, m_GeometryPassPSO.pipelineLayout, 0, 1);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPassPSO.pipeline);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {

		Assets::Model& Model = *m_Models[ModelIndex].get();

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		SamplePushConstants.Model = Model.GetModelMatrix();

		for (const auto& Mesh: Model.Meshes) {
            SamplePushConstants.MaterialIndex = Mesh.MaterialIndex;

            vkCmdPushConstants(commandBuffer, m_GeometryPassPSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &SamplePushConstants);

			vkCmdDrawIndexed(
				commandBuffer, 
				static_cast<uint32_t>(Mesh.Indices.size()), 
				1, 
				static_cast<uint32_t>(Mesh.IndexOffset), 
				static_cast<int32_t>(Mesh.VertexOffset),
				0);
		}
	}

    m_GeometryPassRenderTarget->End(commandBuffer);
}

void AmbientOcclusion::SSAOPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::SSAOPass");

    FullScreenPass(
        commandBuffer, 
        m_SSAOSet[currentFrame],
        m_SSAOPassPSO.pipelineLayout, 
        m_SSAOPassPSO.pipeline, 
        m_SSAORenderTarget.get());
}

void AmbientOcclusion::SSAOBlurPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::SSAOBlurPass");

    FullScreenPass(
        commandBuffer, 
        m_SSAOBlurSet[currentFrame],
        m_SSAOBlurPSO.pipelineLayout, 
        m_SSAOBlurPSO.pipeline, 
        m_SSAOBlurRenderTarget.get());
}

void AmbientOcclusion::SSAOLightCompositionPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::SSAOLightCompositionPass");

    FullScreenPass(
        commandBuffer, 
        m_BlurSSAOEnabled ? m_LightCompositionSetWithSSAOBlur[currentFrame] : m_LightCompositionSetWithoutSSAOBlur[currentFrame],
        m_LightCompositionPSO.pipelineLayout, 
        m_LightCompositionPSO.pipeline, 
        m_LightCompositionRenderTarget.get());
}

void AmbientOcclusion::SSAOPostEffectsPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::SSAOPostEffectsPass");

    FullScreenPass(
        commandBuffer, 
        m_PostEffectsSSAOPassSet[currentFrame], 
        m_PostEffectsPSO.pipelineLayout, 
        m_PostEffectsPSO.pipeline, 
        m_PostEffectsRenderTarget.get());
}

void AmbientOcclusion::FullScreenPass(
    const VkCommandBuffer& commandBuffer, 
    VkDescriptorSet& set,
    const VkPipelineLayout& pipelineLayout,
    const VkPipeline& pipeline,
    Graphics::IRenderTarget* renderTarget) {

    if (!renderTarget) return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    renderTarget->Begin(commandBuffer);

    gfxDevice->BindDescriptorSet(set, commandBuffer, pipelineLayout, 0, 1);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    renderTarget->End(commandBuffer);
}

void AmbientOcclusion::RenderNormalScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

    SCOPED_PROFILER_US("AmbientOcclusion::RenderNormalScene");

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    ForwardLightCompositionPass(currentFrame, commandBuffer);

    Graphics::GPUImage* lastImage = &m_ForwardResolveBuffer;

    if (m_RenderPostEffects) {
        ForwardPostEffectsPass(currentFrame, commandBuffer);
        lastImage = &m_PostEffectsBuffer;
    }

    if (m_FinalImageFirstFrame) {
        gfxDevice->TransitionImageLayout(*lastImage, Graphics::ResourceState::UNDEFINED, Graphics::ResourceState::COPY_SRC);
        m_FinalImageFirstFrame = false;
    } else {
        gfxDevice->TransitionImageLayout(*lastImage, Graphics::ResourceState::SHADER_RESOURCE, Graphics::ResourceState::COPY_SRC);
    }

    gfxDevice->GetSwapChain().RenderTarget->CopyColor(*lastImage);
}

void AmbientOcclusion::ForwardLightCompositionPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::ForwardLightCompositionPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_ForwardPassOffscreenRenderTarget->Begin(commandBuffer);

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

	m_ForwardPassOffscreenRenderTarget->End(commandBuffer);

}

void AmbientOcclusion::ForwardPostEffectsPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::ForwardPostEffectsPass");

    FullScreenPass(
        commandBuffer, 
        m_PostEffectsForwardPassSet[currentFrame],
        m_PostEffectsPSO.pipelineLayout, 
        m_PostEffectsPSO.pipeline, 
        m_PostEffectsRenderTarget.get());
}

void AmbientOcclusion::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
    SCOPED_PROFILER_US("AmbientOcclusion::RenderScene");

    if (m_RenderSSAO) {
        RenderSSAO(currentFrame, commandBuffer);
    } else {
        RenderNormalScene(currentFrame, commandBuffer);
    }
}

void AmbientOcclusion::RenderUI() {
	ImGui::SeparatorText("Scene Settings");
    m_Camera.OnUIRender("Main Camera - Settings");

    ImGui::SeparatorText("SSAO");
    ImGui::Checkbox("Render SSAO", &m_RenderSSAO);
    ImGui::DragInt("SSAO Sample Count", &SSAOUBOData.KernelSize, 1, 1, 64);
    ImGui::DragFloat("SSAO Radius", &SSAOUBOData.Radius, 0.002f, 0.0f, 10.0f);
    ImGui::DragFloat("SSAO Bias", &SSAOUBOData.Bias, 0.002f, 0.0f, 10.0f);

    ImGui::Checkbox("SSAO - Blur enabled", &m_BlurSSAOEnabled);

    bool rangeCheckEnabled = (SSAOUBOData.Flags & (1 << 0));

    ImGui::Checkbox("SSAO - Range Check enabled", &rangeCheckEnabled);

    SSAOUBOData.Flags = (rangeCheckEnabled << 0); 

    ImGui::Checkbox("SSAO - Debug View enabled", &m_SSAODebugViewEnabled);

    ImGui::DragFloat4("Light Direction", (float*)&m_Light, 0.02f, -20.0f, 20.0f);
    ImGui::DragFloat("Light Intensity", &m_Light.w, 0.002f, 0.0f, 1.0f);
    ImGui::DragFloat("Specular Factor", &SamplePushConstants.SpecularFactor, 0.002f, 0.0f, 64.0f);

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

    Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

    InititalizeDisplaySizeDependentResources(m_ScreenWidth, m_ScreenHeight);

    for (uint32_t frameIndex = 0; frameIndex < Graphics::FRAMES_IN_FLIGHT; frameIndex++) {
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[1], m_PostEffectsForwardPassSet[frameIndex], m_ForwardResolveBuffer);
        gfxDevice->WriteDescriptor(m_PostEffectsInputLayout.bindings[1], m_PostEffectsSSAOPassSet[frameIndex], m_LightCompositionBuffer);

        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[1], m_SSAOSet[frameIndex], m_GeometryPositionBuffer);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[2], m_SSAOSet[frameIndex], m_GeometryNormalBuffer);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[3], m_SSAOSet[frameIndex], m_GeometryAlbedoBuffer);
        gfxDevice->WriteDescriptor(m_SSAOInputLayout.bindings[4], m_SSAOSet[frameIndex], m_SSAONoise);

        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[1], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_GeometryPositionBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[2], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_GeometryNormalBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[3], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_GeometryAlbedoBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[4], m_LightCompositionSetWithoutSSAOBlur[frameIndex], m_SSAOBuffer);

        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[1], m_LightCompositionSetWithSSAOBlur[frameIndex], m_GeometryPositionBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[2], m_LightCompositionSetWithSSAOBlur[frameIndex], m_GeometryNormalBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[3], m_LightCompositionSetWithSSAOBlur[frameIndex], m_GeometryAlbedoBuffer);
        gfxDevice->WriteDescriptor(m_LightCompositionInputLayout.bindings[4], m_LightCompositionSetWithSSAOBlur[frameIndex], m_SSAOBlurBuffer);

        gfxDevice->WriteDescriptor(m_SSAOBlurInputLayout.bindings[0], m_SSAOBlurSet[frameIndex], m_SSAOBuffer);
    }    

	m_ForwardPassOffscreenRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_ForwardPassDescription);
    m_PostEffectsRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_PostEffectsPassDescription);
    m_GeometryPassRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_GeometryPassDescription);
    m_SSAORenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_SSAOPassDescription);
    m_LightCompositionRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_LightCompositionPassDescription);
    m_SSAOBlurRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, m_SSAOBlurRenderPassDescription);

    m_FinalImageFirstFrame = true;
}

RUN_APPLICATION(AmbientOcclusion);
