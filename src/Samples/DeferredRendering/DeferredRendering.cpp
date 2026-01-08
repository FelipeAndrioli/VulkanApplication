#include <iostream>

#include "../../src/Core/VulkanHeader.h"
#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/Graphics.h"
#include "../../src/Core/RenderTarget.h"
#include "../../src/Core/Profiler.h"
#include "../../src/Core/ResourceManager.h"
#include "../../src/Core/SceneComponents.h"

#include "../../src/Utils/ModelLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

constexpr int MAX_MODELS = 50;
constexpr int MAX_LIGHTS = 50;

#define ARRAY_SIZE(array) { (uint32_t)(sizeof(array) / sizeof(array[0])) };

class DeferredRendering : public Application::IScene {
public:
	DeferredRendering() {
		settings.Title = "DeferredRendering.exe";
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

	// For comparison
	struct ForwardResourcesResources {

		VkDescriptorSetLayout SetLayout = VK_NULL_HANDLE;
		std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> Set = { VK_NULL_HANDLE };

		Graphics::InputLayout PipelineInputLayout = {};
		Graphics::PipelineState PSO		= {};
		Graphics::Shader VertexShader	= {};
		Graphics::Shader FragShader		= {};
		std::unique_ptr<Graphics::OffscreenRenderTarget> RenderTarget;
	} ForwardResources;

	struct DefererredRenderingResources {

		struct GeometryBuffer {
			Graphics::GPUImage Position;
			Graphics::GPUImage Normals;
			Graphics::GPUImage AlbedoSpec;
			Graphics::GPUImage Depth;
		} GBufferAttachments;

		struct CompositionBuffer {
			Graphics::GPUImage Color;
		} CompositionBufferAttachments;

		struct CombinedForwardBuffer {
			Graphics::GPUImage Depth;
		} CombinedForwardBufferAttachments;

		// --- Geometry Pass Resources ---
		VkDescriptorSetLayout SetLayout = VK_NULL_HANDLE;
		std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> Set = { VK_NULL_HANDLE };

		Graphics::InputLayout GeometryPassInputLayout	= {};
		Graphics::PipelineState GeometryPassPSO			= {};
		Graphics::Shader GeometryPassVertexShader		= {};
		Graphics::Shader GeometryPassFragShader			= {};

		Graphics::RenderPassDescription GeometryPassDescription = {};
		std::unique_ptr<Graphics::MultiAttachmentRenderTarget> GBufferRenderTarget;

		VkDescriptorSetLayout GBufferDisplayDescriptorSetLayout	= VK_NULL_HANDLE;
		std::array<VkDescriptorSet, 3> GBufferDisplayDescriptorSet = { VK_NULL_HANDLE };
		// --- Geometry Pass Resources ---

		// --- Composition Pass Resources ---
		VkDescriptorSetLayout CompositionSetLayout = VK_NULL_HANDLE;
		std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> CompositionSet = { VK_NULL_HANDLE };

		Graphics::InputLayout CompositionPassInputLayout	= {};
		Graphics::PipelineState CompositionPassPSO			= {};
		Graphics::Shader CompositionPassVertexShader		= {};
		Graphics::Shader CompositionPassFragmentShader		= {};

		Graphics::RenderPassDescription CompositionPassDescription = {};
		std::unique_ptr<Graphics::MultiAttachmentRenderTarget> CompositionRenderTarget;
		// --- Composition Pass Resources ---

		// --- Forward Combined Pass Resources ---
		Graphics::RenderPassDescription ForwardCombinedPassDescription = {};
		std::unique_ptr<Graphics::MultiAttachmentRenderTarget> ForwardCombinedRenderTarget;
		// --- Forward Combined Pass Resources ---
	} DeferredResources;

	struct SceneData {
		alignas(16) glm::mat4 Projection;
		alignas(16) glm::mat4 View;
		alignas(16) glm::vec4 extra[6];
		alignas(16) glm::vec4 ViewPosition;
		alignas(4) int TotalLights;
		int extra1;
		int extra2;
		int extra3;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model;
		alignas(4) uint32_t MaterialIndex;
	} SamplePushConstants;

	struct LightSourcesPushConstants {
		glm::mat4 Model;
		glm::vec4 LightColor;
	} LightSourcePushConstants;

	const glm::vec3 InitialCameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= -90.0f;
	const float InitialCameraPitch	= 0.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth	= 0;
	uint32_t m_ScreenHeight = 0;

	const uint32_t m_GBufferSampleCount = 1;
	const uint32_t m_LightingCompositionSampleCount = 1;
	const uint32_t m_ForwardCombinedSampleCount = 1;

	std::array<std::shared_ptr<Assets::Model>, MAX_MODELS> m_Models;
	std::array<Scene::LightComponent, MAX_LIGHTS> m_Lights;

	size_t TotalModels = 0;
	size_t TotalLights = 0;

	Graphics::Buffer m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT] = {};
	Graphics::Buffer m_LightBuffer = {};

	Graphics::PipelineState m_LightSourcesPSODeferred = {};
	Graphics::PipelineState m_LightSourcesPSOForward = {};
	Graphics::Shader m_LightSourcesVertexShader = {};
	Graphics::Shader m_LightSourcesFragmentShader = {};

	VkDescriptorSetLayout m_LightSourcesSetLayout = VK_NULL_HANDLE;
	std::array<VkDescriptorSet, Graphics::FRAMES_IN_FLIGHT> m_LightSourcesSet = { VK_NULL_HANDLE };

	bool m_DeferredRenderingEnabled = false;
	bool m_FirstFrame = true;
	bool m_FirstDeferredPassFrame = true;

private:
	void InitializeForwardResources();
	void InitializeDeferredPassResources();
	void InitializeLightSourcesRenderResources();
	void InitializeDeferredSizeDependentResources(uint32_t width, uint32_t height);

	void DestroyForwardResources();
	void DestroyDeferredResources();

	void RenderForward(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void RenderDeferred(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);

	void DeferredGeometryPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void DeferredLightingCompositionPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void DeferredForwardCombinedPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);

	void AddLight(glm::vec3 position);
	void RemoveLight();
};

void DeferredRendering::AddLight(glm::vec3 position = glm::vec3(0.0f)) {
	if (TotalLights + 1 > MAX_LIGHTS)
		return;

	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
	float lightIntensity = 0.7f;

	Scene::LightComponent& Light = m_Lights[TotalLights];

	Light.ambient				= 0.1f;
	Light.diffuse				= 1.0f;
	Light.specular				= 1.0f;
	Light.position				= glm::vec4(position, 0.0f);
	Light.scale					= 0.05f;
	Light.color					= glm::vec4(r, g, b, lightIntensity);
//	Light.linearAttenuation		= 0.006f;
//	Light.quadraticAttenuation	= 0.007f;

// Learn OpenGL suggestion
//	Light.linearAttenuation		= 0.7f;
//	Light.quadraticAttenuation	= 1.8f;


	Light.linearAttenuation		= 10.0f;
	Light.quadraticAttenuation	= 3.0f;
	TotalLights++;

	SampleSceneData.TotalLights = TotalLights;
}

void DeferredRendering::RemoveLight() {
	if (TotalLights == 0)
		return;

	TotalLights--;

	SampleSceneData.TotalLights = TotalLights;
}


void DeferredRendering::InitializeForwardResources() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	ResourceManager* rm = ResourceManager::Get();

	const uint32_t totalLoadedTextures = static_cast<uint32_t>(rm->GetTotalTextures());

	ForwardResources.PipelineInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			// Scene GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },										// Material GPU Data
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalLoadedTextures, VK_SHADER_STAGE_FRAGMENT_BIT},				// Textures 
			{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}										// Light GPU Data 
		}
	};

	gfxDevice->CreateDescriptorSetLayout(ForwardResources.SetLayout, ForwardResources.PipelineInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(ForwardResources.SetLayout, ForwardResources.Set[i]);

		gfxDevice->WriteDescriptor(ForwardResources.PipelineInputLayout.bindings[0], ForwardResources.Set[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(ForwardResources.PipelineInputLayout.bindings[1], ForwardResources.Set[i], rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(ForwardResources.PipelineInputLayout.bindings[2], ForwardResources.Set[i], rm->GetTextures());
		gfxDevice->WriteDescriptor(ForwardResources.PipelineInputLayout.bindings[3], ForwardResources.Set[i], m_LightBuffer);
	}

	ForwardResources.RenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, ForwardResources.VertexShader, "../src/Samples/DeferredRendering/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, ForwardResources.FragShader, "../src/Samples/DeferredRendering/fragment.glsl");

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Forward Rendering - Phong";
	desc.vertexShader = &ForwardResources.VertexShader;
	desc.fragmentShader = &ForwardResources.FragShader;
	desc.psoInputLayout.push_back(ForwardResources.PipelineInputLayout);

	gfxDevice->CreatePipelineState(desc, ForwardResources.PSO, *ForwardResources.RenderTarget.get());
}

void DeferredRendering::InitializeDeferredSizeDependentResources(uint32_t width, uint32_t height) {

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.Position);
	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.Normals);
	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.AlbedoSpec);
	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.Depth);

	gfxDevice->CreateRenderTarget(
		DeferredResources.GBufferAttachments.Position, 
		Graphics::Format::R16G16B16A16_FLOAT,
		m_ScreenWidth,
		m_ScreenHeight,
		m_GBufferSampleCount);

	gfxDevice->CreateRenderTarget(
		DeferredResources.GBufferAttachments.Normals,
		Graphics::Format::R16G16B16A16_FLOAT,
		m_ScreenWidth,
		m_ScreenHeight,
		m_GBufferSampleCount);

	gfxDevice->CreateRenderTarget(
		DeferredResources.GBufferAttachments.AlbedoSpec,
		Graphics::Format::R8G8B8A8_UNORM,
		m_ScreenWidth,
		m_ScreenHeight,
		m_GBufferSampleCount);

	gfxDevice->CreateDepthBuffer(
		DeferredResources.GBufferAttachments.Depth,
		gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
		m_ScreenWidth, 
		m_ScreenHeight, 
		m_GBufferSampleCount);				

	DeferredResources.GeometryPassDescription.Attachments.clear();

	DeferredResources.GeometryPassDescription.Attachments.push_back(
		Graphics::RenderPassAttachment::RenderTarget(
			DeferredResources.GBufferAttachments.Position,
			Graphics::Format::R16G16B16A16_FLOAT,
			m_GBufferSampleCount,
			Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
			Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
			Graphics::ResourceState::UNDEFINED,
			Graphics::ResourceState::RENDERTARGET,
			Graphics::ResourceState::SHADER_RESOURCE));

	DeferredResources.GeometryPassDescription.Attachments.push_back(
		Graphics::RenderPassAttachment::RenderTarget(
			DeferredResources.GBufferAttachments.Normals,
			Graphics::Format::R16G16B16A16_FLOAT,
			m_GBufferSampleCount,
			Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
			Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
			Graphics::ResourceState::UNDEFINED,
			Graphics::ResourceState::RENDERTARGET,
			Graphics::ResourceState::SHADER_RESOURCE));

	DeferredResources.GeometryPassDescription.Attachments.push_back(
		Graphics::RenderPassAttachment::RenderTarget(
			DeferredResources.GBufferAttachments.AlbedoSpec,
			Graphics::Format::R8G8B8A8_UNORM, 
			m_GBufferSampleCount,
			Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
			Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
			Graphics::ResourceState::UNDEFINED,
			Graphics::ResourceState::RENDERTARGET,
			Graphics::ResourceState::SHADER_RESOURCE));

	DeferredResources.GeometryPassDescription.Attachments.push_back(
		Graphics::RenderPassAttachment::DepthStencil(
			DeferredResources.GBufferAttachments.Depth,
			gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
			m_GBufferSampleCount,
			Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
			Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
			Graphics::ResourceState::UNDEFINED,
			Graphics::ResourceState::DEPTHSTENCIL,
			Graphics::ResourceState::COPY_SRC));

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->WriteDescriptor(DeferredResources.CompositionPassInputLayout.bindings[2], DeferredResources.CompositionSet[i], DeferredResources.GBufferAttachments.Position);
		gfxDevice->WriteDescriptor(DeferredResources.CompositionPassInputLayout.bindings[3], DeferredResources.CompositionSet[i], DeferredResources.GBufferAttachments.Normals);
		gfxDevice->WriteDescriptor(DeferredResources.CompositionPassInputLayout.bindings[4], DeferredResources.CompositionSet[i], DeferredResources.GBufferAttachments.AlbedoSpec);
	}

	gfxDevice->DestroyImage(DeferredResources.CompositionBufferAttachments.Color);

	gfxDevice->CreateRenderTarget(
		DeferredResources.CompositionBufferAttachments.Color,
		Graphics::Format::R8G8B8A8_UNORM,
		m_ScreenWidth,
		m_ScreenHeight,
		m_LightingCompositionSampleCount);

	DeferredResources.CompositionPassDescription.Attachments.clear();

	DeferredResources.CompositionPassDescription.Attachments.push_back(
		Graphics::RenderPassAttachment::RenderTarget(
			DeferredResources.CompositionBufferAttachments.Color,
			Graphics::Format::R8G8B8A8_UNORM,
			m_LightingCompositionSampleCount,
			Graphics::RenderPassAttachment::AttachmentLoadOp::CLEAR,
			Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
			Graphics::ResourceState::UNDEFINED,
			Graphics::ResourceState::RENDERTARGET,
			Graphics::ResourceState::RENDERTARGET));

	gfxDevice->DestroyImage(DeferredResources.CombinedForwardBufferAttachments.Depth);

	gfxDevice->CreateDepthBuffer(
		DeferredResources.CombinedForwardBufferAttachments.Depth,
		gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
		m_ScreenWidth,
		m_ScreenHeight,
		m_ForwardCombinedSampleCount);

	DeferredResources.ForwardCombinedPassDescription.Attachments.clear();

	DeferredResources.ForwardCombinedPassDescription.Attachments.push_back(
		Graphics::RenderPassAttachment::RenderTarget(
			DeferredResources.CompositionBufferAttachments.Color,
			Graphics::Format::R8G8B8A8_UNORM,
			m_LightingCompositionSampleCount,
			Graphics::RenderPassAttachment::AttachmentLoadOp::LOAD,
			Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
			Graphics::ResourceState::RENDERTARGET,
			Graphics::ResourceState::RENDERTARGET,
			Graphics::ResourceState::COPY_SRC));

	DeferredResources.ForwardCombinedPassDescription.Attachments.push_back(
		Graphics::RenderPassAttachment::DepthStencil(
			DeferredResources.CombinedForwardBufferAttachments.Depth,
			gfxDevice->ConvertFormat(gfxDevice->GetDepthFormat()),
			m_ForwardCombinedSampleCount,
			Graphics::RenderPassAttachment::AttachmentLoadOp::LOAD,
			Graphics::RenderPassAttachment::AttachmentStoreOp::STORE,
			Graphics::ResourceState::DEPTHSTENCIL,
			Graphics::ResourceState::DEPTHSTENCIL,
			Graphics::ResourceState::DEPTHSTENCIL));

	Graphics::InputLayout dummyInputLayout = {
		.pushConstants = {},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		}
	};

	gfxDevice->DestroyDescriptorSetLayout(DeferredResources.GBufferDisplayDescriptorSetLayout);
	gfxDevice->CreateDescriptorSetLayout(DeferredResources.GBufferDisplayDescriptorSetLayout, dummyInputLayout.bindings);

	gfxDevice->CreateDescriptorSet(DeferredResources.GBufferDisplayDescriptorSetLayout, DeferredResources.GBufferDisplayDescriptorSet[0]);
	gfxDevice->WriteDescriptor(dummyInputLayout.bindings[0], DeferredResources.GBufferDisplayDescriptorSet[0], DeferredResources.GBufferAttachments.Position);

	gfxDevice->CreateDescriptorSet(DeferredResources.GBufferDisplayDescriptorSetLayout, DeferredResources.GBufferDisplayDescriptorSet[1]);
	gfxDevice->WriteDescriptor(dummyInputLayout.bindings[0], DeferredResources.GBufferDisplayDescriptorSet[1], DeferredResources.GBufferAttachments.Normals);

	gfxDevice->CreateDescriptorSet(DeferredResources.GBufferDisplayDescriptorSetLayout, DeferredResources.GBufferDisplayDescriptorSet[2]);
	gfxDevice->WriteDescriptor(dummyInputLayout.bindings[0], DeferredResources.GBufferDisplayDescriptorSet[2], DeferredResources.GBufferAttachments.AlbedoSpec);
}

void DeferredRendering::InitializeDeferredPassResources() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	ResourceManager* rm = ResourceManager::Get();

	const uint32_t totalLoadedTextures = static_cast<uint32_t>(rm->GetTotalTextures());

	DeferredResources.GeometryPassInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },			// Scene GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },										// Material GPU Data
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalLoadedTextures, VK_SHADER_STAGE_FRAGMENT_BIT}				// Textures 
		}
	};

	gfxDevice->CreateDescriptorSetLayout(DeferredResources.SetLayout, DeferredResources.GeometryPassInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(DeferredResources.SetLayout, DeferredResources.Set[i]);

		gfxDevice->WriteDescriptor(DeferredResources.GeometryPassInputLayout.bindings[0], DeferredResources.Set[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(DeferredResources.GeometryPassInputLayout.bindings[1], DeferredResources.Set[i], rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(DeferredResources.GeometryPassInputLayout.bindings[2], DeferredResources.Set[i], rm->GetTextures());
	}

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, DeferredResources.GeometryPassVertexShader, "../src/Samples/DeferredRendering/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, DeferredResources.GeometryPassFragShader, "../src/Samples/DeferredRendering/deferred_geometry_fragment.glsl");

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Deferred Rendering - Geometry Pass";
	desc.vertexShader = &DeferredResources.GeometryPassVertexShader;
	desc.fragmentShader = &DeferredResources.GeometryPassFragShader;
	desc.attachmentCount = 3;	// Position, Normal, AlbedoSpec
	desc.psoInputLayout.push_back(DeferredResources.GeometryPassInputLayout);

	DeferredResources.CompositionPassInputLayout = {
		.pushConstants = {

		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },		// Scene UBO
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },		// Lighting UBO
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },		// Position 
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },		// Normal 
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }		// AlbedoSpec 
		}
	};

	gfxDevice->CreateDescriptorSetLayout(DeferredResources.CompositionSetLayout, DeferredResources.CompositionPassInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(DeferredResources.CompositionSetLayout, DeferredResources.CompositionSet[i]);

		gfxDevice->WriteDescriptor(DeferredResources.CompositionPassInputLayout.bindings[0], DeferredResources.CompositionSet[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(DeferredResources.CompositionPassInputLayout.bindings[1], DeferredResources.CompositionSet[i], m_LightBuffer);
	}

	InitializeDeferredSizeDependentResources(m_ScreenWidth, m_ScreenHeight);

	DeferredResources.GBufferRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
		m_ScreenWidth,
		m_ScreenHeight,
		m_GBufferSampleCount,
		DeferredResources.GeometryPassDescription);

	DeferredResources.CompositionRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
		m_ScreenWidth, 
		m_ScreenHeight, 
		m_LightingCompositionSampleCount,
		DeferredResources.CompositionPassDescription);

	DeferredResources.ForwardCombinedRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(
		m_ScreenWidth,
		m_ScreenHeight,
		m_ForwardCombinedSampleCount,
		DeferredResources.ForwardCombinedPassDescription);

	gfxDevice->CreatePipelineState(desc, DeferredResources.GeometryPassPSO, *DeferredResources.GBufferRenderTarget.get());

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, DeferredResources.CompositionPassVertexShader, "../src/Samples/DeferredRendering/deferred_lighting_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, DeferredResources.CompositionPassFragmentShader, "../src/Samples/DeferredRendering/deferred_lighting_fragment.glsl");

	Graphics::PipelineStateDescription lightingPsoDesc = {};
	lightingPsoDesc.Name = "Deferred Rendering - Lighting Composition Pass";
	lightingPsoDesc.noVertex = true;
	lightingPsoDesc.cullMode = VK_CULL_MODE_NONE;
	lightingPsoDesc.vertexShader = &DeferredResources.CompositionPassVertexShader;
	lightingPsoDesc.fragmentShader = &DeferredResources.CompositionPassFragmentShader;
	lightingPsoDesc.psoInputLayout.push_back(DeferredResources.CompositionPassInputLayout);

	gfxDevice->CreatePipelineState(lightingPsoDesc, DeferredResources.CompositionPassPSO, *DeferredResources.CompositionRenderTarget.get());
}

void DeferredRendering::InitializeLightSourcesRenderResources() {

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	Graphics::InputLayout lightSourcesInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(LightSourcesPushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }		// Scene GPU Data
		}
	};

	gfxDevice->CreateDescriptorSetLayout(m_LightSourcesSetLayout, lightSourcesInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_LightSourcesSetLayout, m_LightSourcesSet[i]);
		gfxDevice->WriteDescriptor(lightSourcesInputLayout.bindings[0], m_LightSourcesSet[i], m_SceneBuffer[i]);
	}

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourcesVertexShader, "../src/Samples/DeferredRendering/light_source_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourcesFragmentShader, "../src/Samples/DeferredRendering/light_source_fragment.glsl");

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Light Sources";
	desc.vertexShader = &m_LightSourcesVertexShader;
	desc.fragmentShader = &m_LightSourcesFragmentShader;
	desc.noVertex = true;
	desc.psoInputLayout.push_back(lightSourcesInputLayout);

	gfxDevice->CreatePipelineState(desc, m_LightSourcesPSODeferred, *DeferredResources.ForwardCombinedRenderTarget.get());
	gfxDevice->CreatePipelineState(desc, m_LightSourcesPSOForward, *ForwardResources.RenderTarget.get());
}

void DeferredRendering::StartUp() {

	m_ScreenWidth = settings.Width;
	m_ScreenHeight = settings.Height;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

	for (size_t i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		m_SceneBuffer[i] = gfxDevice->CreateBuffer(sizeof(SceneData));
	}

	m_LightBuffer = gfxDevice->CreateBuffer(sizeof(Scene::LightComponent) * MAX_LIGHTS);

	const int maxRow = 5;
	const int maxColumn = 5;

	const float initialX = 0.0f - (maxRow / 2.0f);
	const float initialZ = 0.0f - (maxColumn / 2.0f);

	const float offsetIncrease = 0.5f;

	float offsetX = 0.0f;
	float offsetZ = 0.0f;

	uint32_t originalModelIndex = 0;

	for (size_t i = 0; i < maxRow && TotalModels < MAX_MODELS; i++) {

		float x = i + initialX + offsetX;

		for (size_t j = 0; j < maxColumn && TotalModels < MAX_MODELS; j++) {
			
			float z = j + initialZ + offsetZ;

			if (i == 0 && j == 0) {
				m_Models[TotalModels] = ModelLoader::LoadModel("C:/Users/felip/Documents/current_projects/models/actual_models/backpack/backpack.obj");
				ModelLoader::FlipModelUvVertically(*m_Models[TotalModels].get());
				originalModelIndex = TotalModels;
			}
			else {
				m_Models[TotalModels] = ModelLoader::DuplicateModel(m_Models[originalModelIndex]);
			}

			m_Models[TotalModels]->Transformations.scaleHandler		= 0.3f;
			m_Models[TotalModels]->Transformations.translation		= glm::vec3(x, 0.6f, z);
			m_Models[TotalModels]->FlipUvVertically					= true;
			m_Models[TotalModels]->ModelIndex						= TotalModels;
			
			TotalModels++;

			offsetZ += offsetIncrease;
		}

		offsetZ = 0.0f;
		offsetX += offsetIncrease;
	}

	const int maxColumns = 5;

	int currentColumnCount = 0;
	int currentRowCount = 0;

	const float lightInitialX = -7.0f;
	const float lightInitialZ = -7.0f;

	for (size_t i = 0; i < MAX_LIGHTS; i++) {

		float distance = 5.0f;

		float x = lightInitialX + distance + currentRowCount;
		float y = 1.0f;
		float z = lightInitialZ + distance + currentColumnCount;

		if (++currentColumnCount == maxColumns) {
			currentColumnCount = 0;
			currentRowCount++;
		}

		AddLight(glm::vec3(x, y, z));
	}

	InitializeForwardResources();
	InitializeDeferredPassResources();
	InitializeLightSourcesRenderResources();
}

void DeferredRendering::DestroyForwardResources() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	ForwardResources.RenderTarget.reset();

	gfxDevice->DestroyShader(ForwardResources.VertexShader);
	gfxDevice->DestroyShader(ForwardResources.FragShader);
	gfxDevice->DestroyDescriptorSetLayout(ForwardResources.SetLayout);
	gfxDevice->DestroyPipeline(ForwardResources.PSO);
}

void DeferredRendering::DestroyDeferredResources() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.Position);
	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.Normals);
	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.AlbedoSpec);
	gfxDevice->DestroyImage(DeferredResources.GBufferAttachments.Depth);

	DeferredResources.GBufferRenderTarget.reset();

	gfxDevice->DestroyShader(DeferredResources.GeometryPassVertexShader);
	gfxDevice->DestroyShader(DeferredResources.GeometryPassFragShader);
	gfxDevice->DestroyDescriptorSetLayout(DeferredResources.SetLayout);
	gfxDevice->DestroyPipeline(DeferredResources.GeometryPassPSO);

	gfxDevice->DestroyImage(DeferredResources.CompositionBufferAttachments.Color);

	DeferredResources.CompositionRenderTarget.reset();

	gfxDevice->DestroyShader(DeferredResources.CompositionPassVertexShader);
	gfxDevice->DestroyShader(DeferredResources.CompositionPassFragmentShader);
	gfxDevice->DestroyDescriptorSetLayout(DeferredResources.CompositionSetLayout);
	gfxDevice->DestroyPipeline(DeferredResources.CompositionPassPSO);

	gfxDevice->DestroyImage(DeferredResources.CombinedForwardBufferAttachments.Depth);

	DeferredResources.ForwardCombinedRenderTarget.reset();

	gfxDevice->DestroyDescriptorSetLayout(DeferredResources.GBufferDisplayDescriptorSetLayout);
}

void DeferredRendering::CleanUp() {
	DestroyForwardResources();
	DestroyDeferredResources();

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyShader(m_LightSourcesVertexShader);
	gfxDevice->DestroyShader(m_LightSourcesFragmentShader);
	gfxDevice->DestroyDescriptorSetLayout(m_LightSourcesSetLayout);
	gfxDevice->DestroyPipeline(m_LightSourcesPSOForward);
	gfxDevice->DestroyPipeline(m_LightSourcesPSODeferred);
}

void DeferredRendering::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("DeferredRendering::Update");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	m_Camera.OnUpdate(deltaT, input);

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
	SampleSceneData.ViewPosition	= glm::vec4(m_Camera.Position, 1.0f);
	SampleSceneData.TotalLights		= TotalLights;

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], &SampleSceneData);

	for (size_t LightIndex = 0; LightIndex < TotalLights; ++LightIndex) {
		Scene::LightComponent& light = m_Lights[LightIndex];

		glm::mat4 toOrigin		= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f));
		glm::mat4 scale			= glm::scale(glm::mat4(1.0f), glm::vec3(light.scale));
		glm::mat4 toPosition	= glm::translate(glm::mat4(1.0f), glm::vec3(light.position));

		light.model = toPosition * scale * toOrigin;
		light.radius = Scene::CalculateLightRadius(light);
	}

	gfxDevice->UpdateBuffer(m_LightBuffer, m_Lights.data());
}

void DeferredRendering::RenderForward(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("DeferredRendering::RenderForward");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	ForwardResources.RenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(ForwardResources.Set[currentFrame], commandBuffer, ForwardResources.PSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ForwardResources.PSO.pipeline);

	for (int ModelIndex = 0; ModelIndex < TotalModels; ModelIndex++) {

		Assets::Model& Model = *m_Models[ModelIndex].get();

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		SamplePushConstants.Model = Model.GetModelMatrix();

		for (const auto& Mesh : Model.Meshes) {

			SamplePushConstants.MaterialIndex = Mesh.MaterialIndex;

			vkCmdPushConstants(commandBuffer, ForwardResources.PSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &SamplePushConstants);

			vkCmdDrawIndexed(
				commandBuffer,
				static_cast<uint32_t>(Mesh.Indices.size()),
				1,
				static_cast<uint32_t>(Mesh.IndexOffset),
				static_cast<int32_t>(Mesh.VertexOffset),
				0);
		}
	}

	gfxDevice->BindDescriptorSet(m_LightSourcesSet[currentFrame], commandBuffer, m_LightSourcesPSOForward.pipelineLayout, 0, 1);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightSourcesPSOForward.pipeline);

	for (int LightIndex = 0; LightIndex < TotalLights; LightIndex++) {
		LightSourcePushConstants.LightColor = m_Lights[LightIndex].color;
		LightSourcePushConstants.Model = m_Lights[LightIndex].model;

		vkCmdPushConstants(commandBuffer, m_LightSourcesPSOForward.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(LightSourcesPushConstants), &LightSourcePushConstants);
		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}

	ForwardResources.RenderTarget->End(commandBuffer);

	ForwardResources.RenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(ForwardResources.RenderTarget->GetColorBuffer());

}

void DeferredRendering::RenderDeferred(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("DeferredRendering::RenderDeferred");
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	DeferredGeometryPass(currentFrame, commandBuffer);
	DeferredLightingCompositionPass(currentFrame, commandBuffer);
	DeferredForwardCombinedPass(currentFrame, commandBuffer);

	// TODO: find out why the render pass'implicit' layout transition doesn't happen for this render target in the very first usage.
	if (m_FirstDeferredPassFrame) {
		gfxDevice->TransitionImageLayout(
			DeferredResources.CompositionBufferAttachments.Color,
			gfxDevice->ConvertResourceStateToImageLayout(Graphics::ResourceState::UNDEFINED),
			gfxDevice->ConvertResourceStateToImageLayout(Graphics::ResourceState::COPY_SRC));
	}

	gfxDevice->GetSwapChain().RenderTarget->CopyColor(DeferredResources.CompositionBufferAttachments.Color);
}

void DeferredRendering::DeferredGeometryPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("DeferredRendering::DeferredGeometryPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	DeferredResources.GBufferRenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(DeferredResources.Set[currentFrame], commandBuffer, DeferredResources.GeometryPassPSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredResources.GeometryPassPSO.pipeline);

	for (uint32_t ModelIndex = 0; ModelIndex < TotalModels; ModelIndex++) {
		Assets::Model& Model = *m_Models[ModelIndex].get();

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		SamplePushConstants.Model = Model.GetModelMatrix();

		for (const auto& Mesh : Model.Meshes) {
			SamplePushConstants.MaterialIndex = Mesh.MaterialIndex;

			vkCmdPushConstants(commandBuffer, DeferredResources.GeometryPassPSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants), &SamplePushConstants);

			vkCmdDrawIndexed(
				commandBuffer,
				static_cast<uint32_t>(Mesh.Indices.size()),
				1,
				static_cast<uint32_t>(Mesh.IndexOffset),
				static_cast<int32_t>(Mesh.VertexOffset),
				0);
		}
	}

	DeferredResources.GBufferRenderTarget->End(commandBuffer);
}

void DeferredRendering::DeferredLightingCompositionPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("DeferredRendering::DeferredLightingCompositionPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	DeferredResources.CompositionRenderTarget->Begin(commandBuffer);

	// Render scene applying lighting
	gfxDevice->BindDescriptorSet(DeferredResources.CompositionSet[currentFrame], commandBuffer, DeferredResources.CompositionPassPSO.pipelineLayout, 0, 1);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredResources.CompositionPassPSO.pipeline);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);

	DeferredResources.CompositionRenderTarget->End(commandBuffer);
}

void DeferredRendering::DeferredForwardCombinedPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("DeferredRendering::DeferredForwardCombinedPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	// TODO: Move the whole copy image to a better/separate function.
	// Copying depth buffer before rendering - BEGIN 
	{

		if (m_FirstDeferredPassFrame) {
			gfxDevice->TransitionImageLayout(
				DeferredResources.GBufferAttachments.Depth,
				Graphics::ResourceState::UNDEFINED,
				Graphics::ResourceState::COPY_SRC);
		}

		gfxDevice->TransitionImageLayout(
			DeferredResources.CombinedForwardBufferAttachments.Depth,
			Graphics::ResourceState::UNDEFINED,
			Graphics::ResourceState::COPY_DST);

		VkImageCopy imageCopy = {};
		imageCopy.extent.width = m_ScreenWidth;
		imageCopy.extent.height = m_ScreenHeight;
		imageCopy.extent.depth = 1;
		imageCopy.srcOffset = { .x = 0, .y = 0, .z = 0 };
		imageCopy.srcSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		imageCopy.dstOffset = { .x = 0, .y = 0, .z = 0 };
		imageCopy.dstSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		VkCommandBuffer copyCommandBuffer = gfxDevice->BeginSingleTimeCommandBuffer();

		vkCmdCopyImage(copyCommandBuffer,
			DeferredResources.GBufferAttachments.Depth.Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			DeferredResources.CombinedForwardBufferAttachments.Depth.Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopy);

		gfxDevice->EndSingleTimeCommandBuffer(copyCommandBuffer);

		gfxDevice->TransitionImageLayout(
			DeferredResources.CombinedForwardBufferAttachments.Depth,
			Graphics::ResourceState::COPY_DST,
			Graphics::ResourceState::DEPTHSTENCIL);
	}
	// Copying depth buffer before rendering - END 

	DeferredResources.ForwardCombinedRenderTarget->Begin(commandBuffer);

	gfxDevice->BindDescriptorSet(m_LightSourcesSet[currentFrame], commandBuffer, m_LightSourcesPSODeferred.pipelineLayout, 0, 1);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightSourcesPSODeferred.pipeline);

	for (int LightIndex = 0; LightIndex < TotalLights; LightIndex++) {
		LightSourcePushConstants.LightColor = m_Lights[LightIndex].color;
		LightSourcePushConstants.Model = m_Lights[LightIndex].model;

		vkCmdPushConstants(commandBuffer, m_LightSourcesPSODeferred.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(LightSourcesPushConstants), &LightSourcePushConstants);
		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}

	DeferredResources.ForwardCombinedRenderTarget->End(commandBuffer);
}

void DeferredRendering::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("DeferredRendering::RenderScene");

	if (m_DeferredRenderingEnabled) {
		RenderDeferred(currentFrame, commandBuffer);
		m_FirstDeferredPassFrame = false;
	}
	else {
		RenderForward(currentFrame, commandBuffer);
	}

	m_FirstFrame = false;
}

void DeferredRendering::RenderUI() {
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

	if (ImGui::TreeNode("Lights")) {
		if (ImGui::Button("Add Light")) {
			AddLight();
		}

		if (ImGui::Button("Remove Light")) {
			RemoveLight();
		}

		for (size_t LightIndex = 0; LightIndex < TotalLights; LightIndex++) {
			std::string LightId = "Light ";
			LightId = LightId.append(std::to_string(LightIndex));

			if (ImGui::TreeNode(LightId.c_str())) {	
				ImGui::DragFloat4("Light Position", (float*)&m_Lights[LightIndex].position, 0.02f, -20.0f, 20.0f);
				ImGui::DragFloat("Light Scale", &m_Lights[LightIndex].scale, 0.01f, 0.0f, 1.0f);
				ImGui::ColorPicker4("Light Color", (float*)&m_Lights[LightIndex].color);
				ImGui::DragFloat("Light Intensity", (float*)&m_Lights[LightIndex].color.a, 0.02f, 0.0f, 1.0f);
				ImGui::DragFloat("Light Linear Attenuation", &m_Lights[LightIndex].linearAttenuation, 0.02f, -20.0f, 20.0f);
				ImGui::DragFloat("Light Quadratic Attenuation", &m_Lights[LightIndex].quadraticAttenuation, 0.02f, -20.0f, 20.0f);
				ImGui::DragFloat("Light Radius", &m_Lights[LightIndex].radius, 0.02f, 0.0f, 10.0f);
				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Models")) {
		for (size_t ModelIndex = 0; ModelIndex < TotalModels; ++ModelIndex) {
			m_Models[ModelIndex]->OnUIRender();
		}
		
		ImGui::TreePop();
	}

	bool deferredRenderingEnabledBefore = m_DeferredRenderingEnabled;

	ImGui::Checkbox("Deferred Rendering Enabled", &m_DeferredRenderingEnabled);

	if (ImGui::TreeNode("GBuffer")) {
		if (m_DeferredRenderingEnabled && deferredRenderingEnabledBefore == m_DeferredRenderingEnabled) {

			if (ImGui::TreeNode("Position")) {
				ImGui::Image((ImTextureID)DeferredResources.GBufferDisplayDescriptorSet[0], ImVec2(350, 300));
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Normals")) {
				ImGui::Image((ImTextureID)DeferredResources.GBufferDisplayDescriptorSet[1], ImVec2(350, 300));
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("AlbedoSpec")) {
				ImGui::Image((ImTextureID)DeferredResources.GBufferDisplayDescriptorSet[2], ImVec2(350, 300));
				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}

void DeferredRendering::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);

	InitializeDeferredSizeDependentResources(width, height);

	ForwardResources.RenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
	DeferredResources.GBufferRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, DeferredResources.GeometryPassDescription);
	DeferredResources.CompositionRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, DeferredResources.CompositionPassDescription);
	DeferredResources.ForwardCombinedRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight, DeferredResources.ForwardCombinedPassDescription);

	m_FirstDeferredPassFrame = true;
}

RUN_APPLICATION(DeferredRendering);
