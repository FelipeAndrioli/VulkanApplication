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

constexpr int MAX_LIGHTS = 30;
constexpr int TOTAL_MODELS = 3;

class GaussianBlur : public Application::IScene {
public:
	GaussianBlur() {
		settings.Title = "GaussianBlur.exe";
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
		glm::mat4 Projection = glm::mat4(1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::vec4 Extra[7] = {};
		uint32_t TotalLights = 0;
		uint32_t Extra1 = 0;
		uint32_t Extra2 = 0;
		float BloomThreshold = 1.0f;
	} SampleSceneData;

	struct PushConstants {
		alignas(16) glm::mat4 Model;
		alignas(4) uint32_t MaterialIndex;
	} SamplePushConstants;

	struct LightSourceRenderPushConstants {
		alignas(16) glm::mat4 Model;
		alignas(16) glm::vec4 Color;
	} LightSourcePushConstants;

	// Note:	Only point lights for now
	struct Light {
		glm::mat4 Model = glm::mat4(1.0f);			// to render the light sources		| 64 bytes
		glm::vec4 Position = glm::vec4(0.0f);		// 16 bytes							| 80 bytes aggregated
		glm::vec4 Color = glm::vec4(1.0f);			// w -> light intensity | 16 bytes	| 94 bytes aggregated
		glm::vec4 Extra_v[9] = {};
		float Linear = -4.0f;
		float Quadratic = 10.0f;
		float Extra1 = 0.0f;
		float Extra2 = 0.0f;
	};

	const glm::vec3 InitialCameraPosition = glm::vec3(-6.0f, 0.0f, 0.0f);

	const float InitialCameraFov	= 45.0f;
	const float InitialCameraYaw	= 0.0f;
	const float InitialCameraPitch	= 0.0f;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth		= 0;
	uint32_t m_ScreenHeight		= 0;
	uint32_t m_BlurPassCount	= 10;

	std::array<std::shared_ptr<Assets::Model>, TOTAL_MODELS> m_Models;

	size_t m_TotalModels = 0;

	std::array<Light, MAX_LIGHTS> m_SceneLights;

	size_t m_TotalLights = 0;

	struct PostProcessGPUData {
		uint32_t Flags;			// HDR ACES Tone Mapping Enabled | HDR Exposure Enabled | Gamma Correction Enabled | HDR Enabled
		float Gamma;
		float HDRExposure;
		
	} m_PostProcessingGPUData;

	float m_LightScaleFactor	= 0.0f;
	float m_LightPositionOffset = 4.0f;
	float m_Gamma				= 0.0f;
	float m_HDRExposure			= 0.0f;

	bool m_GammaCorrectionEnabled	= false;
	bool m_HDRReinhardEnabled		= false;
	bool m_HDRExposureEnabled		= true;
	bool m_HDRAcesEnabled			= false;
	bool m_MovingLights				= false;
	bool m_BloomEnabled				= true;
	bool m_HorizontalBlurPass		= false;

	std::unique_ptr<Graphics::PostEffectsRenderTarget> m_PostEffectsRenderTarget;
	std::unique_ptr<Graphics::PostEffectsRenderTarget> m_PingPongRenderTarget[2];
	std::unique_ptr<Graphics::MultiAttachmentRenderTarget> m_BloomRenderTarget;

	Graphics::Buffer m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT] = {};
	Graphics::Buffer m_LightBuffer			= {};
	Graphics::Buffer m_PostProcessBuffer	= {};

	// Gaussian Blur Pass
	Graphics::Shader m_GaussianBlurVertexShader = {};
	Graphics::Shader m_GaussianBlurFragmentShader = {};

	Graphics::InputLayout m_GaussianBlurInputLayout = {};

	Graphics::PipelineState m_GaussianBlurPSO = {};

	VkDescriptorSetLayout m_GaussianBlurSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_GaussianBlurSet[Graphics::FRAMES_IN_FLIGHT][3] = { VK_NULL_HANDLE };	// Per frame, per input image type 

	// Geometry Pass
	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::PipelineState m_PSO = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_Set[Graphics::FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };

	// Light Sources
	Graphics::Shader m_LightSourceVertexShader		= {};
	Graphics::Shader m_LightSourceFragmentShader	= {};

	Graphics::PipelineState m_LightSourcePSO = {};

	VkDescriptorSetLayout m_LightSourceSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_LightSourceDescriptorSet[Graphics::FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };

	// Post Effects Pass

	Graphics::Shader m_PostEffectsVertexShader = {};
	Graphics::Shader m_PostEffectsFragShader = {};

	Graphics::InputLayout m_PostEffectsInputLayouts[2] = {};

	Graphics::PipelineState m_PostEffectsPSO = {};
	VkDescriptorSetLayout m_PostEffectsSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_PostEffectsSet[Graphics::FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };
private:
	void RenderSceneGeometry(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void RenderLightSources(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void RenderPostEffects(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void GaussianBlurPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer);
	void AddLight();
	void RemoveLight();
};

void GaussianBlur::AddLight() {

	static const glm::vec4 colors[6] = {
		{ 3.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 3.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 3.0f, 1.0f },
		{ 3.0f, 3.0f, 0.0f, 1.0f },
		{ 0.0f, 3.0f, 3.0f, 1.0f },
		{ 3.0f, 0.0f, 3.0f, 1.0f }
	};

	if (m_TotalLights + 1 >= MAX_LIGHTS)
		return;

	glm::vec4 position = glm::vec4(1.0f);
	position.x = m_TotalLights - m_LightPositionOffset;
	position.y = 0.0f;
	position.z = 1.0f;

	m_SceneLights[m_TotalLights].Position	= position;
	m_SceneLights[m_TotalLights].Color		= colors[m_TotalLights % 6];

	m_TotalLights++;
}

void GaussianBlur::RemoveLight() {
	if (m_TotalLights == 0)
		return;

	m_TotalLights--;
}

void GaussianBlur::StartUp() {
	
	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;

	m_LightScaleFactor = 0.08f;

	m_Gamma = 2.2f;
	m_HDRExposure = 1.0f;

	m_BloomRenderTarget = std::make_unique<Graphics::MultiAttachmentRenderTarget>(m_ScreenWidth, m_ScreenHeight, 2, Graphics::Format::R16G16B16A16_FLOAT);
	m_PostEffectsRenderTarget = std::make_unique<Graphics::PostEffectsRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_PingPongRenderTarget[0] = std::make_unique<Graphics::PostEffectsRenderTarget>(m_ScreenWidth, m_ScreenHeight, Graphics::Format::R16G16B16A16_FLOAT, Graphics::ResourceState::SHADER_RESOURCE);
	m_PingPongRenderTarget[1] = std::make_unique<Graphics::PostEffectsRenderTarget>(m_ScreenWidth, m_ScreenHeight, Graphics::Format::R16G16B16A16_FLOAT, Graphics::ResourceState::SHADER_RESOURCE);

	m_Camera.Init(InitialCameraPosition, InitialCameraFov, InitialCameraYaw, InitialCameraPitch, m_ScreenWidth, m_ScreenHeight);

	for (size_t LightIndex = 0; LightIndex < 15; ++LightIndex) {
		AddLight();
	}

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	ResourceManager* rm = ResourceManager::Get();

	m_Models[m_TotalModels] = ModelLoader::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj");
	m_Models[m_TotalModels]->Transformations.scaleHandler	= 0.008f;
	m_Models[m_TotalModels]->ModelIndex						= static_cast<int>(m_TotalModels);

	m_TotalModels++;

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/GaussianBlur/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/GaussianBlur/fragment.glsl");

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; ++i) {
		m_SceneBuffer[i] = gfxDevice->CreateBuffer(sizeof(SceneData));
	}

	m_LightBuffer		= gfxDevice->CreateBuffer(sizeof(Light) * MAX_LIGHTS);
	m_PostProcessBuffer = gfxDevice->CreateBuffer(sizeof(PostProcessGPUData));

	Graphics::InputLayout inputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },								// Scene UBO
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },															// Materials UBO
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(rm->GetTextures().size()), VK_SHADER_STAGE_FRAGMENT_BIT },	// Textures Samplers
			{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }															// Light UBO
		}
	};

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Offscreen Phong";
	desc.vertexShader = &m_VertexShader;
	desc.fragmentShader = &m_FragShader;
	desc.attachmentCount = m_BloomRenderTarget->GetColorAttachmentCount();

	for (uint32_t i = 0; i < desc.attachmentCount; ++i) {
		desc.colorBlendingDescArray[i] = {};
	}

	desc.psoInputLayout.push_back(inputLayout);

	gfxDevice->CreatePipelineState(desc, m_PSO, *m_BloomRenderTarget.get());
	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, inputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; ++i) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[0], m_Set[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[1], m_Set[i], rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(inputLayout.bindings[2], m_Set[i], rm->GetTextures());
		gfxDevice->WriteDescriptor(inputLayout.bindings[3], m_Set[i], m_LightBuffer);
	}

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourceVertexShader, "../src/Samples/GaussianBlur/light_source_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourceFragmentShader, "../src/Samples/GaussianBlur/light_source_fragment.glsl");

	Graphics::InputLayout lightSourceRenderInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(LightSourceRenderPushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }		// Scene UBO
		}
	};

	desc.Name = "Light Source Rendering PSO";
	desc.vertexShader = &m_LightSourceVertexShader;
	desc.fragmentShader = &m_LightSourceFragmentShader;
	desc.noVertex = true;
	desc.psoInputLayout.clear();
	desc.psoInputLayout.push_back(lightSourceRenderInputLayout);

	gfxDevice->CreatePipelineState(desc, m_LightSourcePSO, *m_BloomRenderTarget.get());
	gfxDevice->CreateDescriptorSetLayout(m_LightSourceSetLayout, lightSourceRenderInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; ++i) {
		gfxDevice->CreateDescriptorSet(m_LightSourceSetLayout, m_LightSourceDescriptorSet[i]);
		gfxDevice->WriteDescriptor(lightSourceRenderInputLayout.bindings[0], m_LightSourceDescriptorSet[i], m_SceneBuffer[i]);
	}

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_PostEffectsVertexShader, "../src/Samples/GaussianBlur/post_effects_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_PostEffectsFragShader, "../src/Samples/GaussianBlur/post_effects_fragment.glsl");

	m_PostEffectsInputLayouts[0] = {
		.pushConstants = {},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },	// Geometry Pass Result
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT	}			// Post Process UBO
		}
	};

	// Instead of duplicating set 0 to support finishing bloom rendering in 'vertical' or 'horizontal', I'll just create a separate
	// 'set' entry for the pipeline so I can directly bind the result to this slot.
	m_PostEffectsInputLayouts[1] = {
		.pushConstants = {},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },	// Bloom Pass Result
		}
	};

	Graphics::PipelineStateDescription postEffectsPsoDesc = {};
	postEffectsPsoDesc.Name = "Post Effects PSO";
	postEffectsPsoDesc.vertexShader = &m_PostEffectsVertexShader;
	postEffectsPsoDesc.fragmentShader = &m_PostEffectsFragShader;
	postEffectsPsoDesc.noVertex = true;
	postEffectsPsoDesc.cullMode = VK_CULL_MODE_NONE;
	postEffectsPsoDesc.psoInputLayout.push_back(m_PostEffectsInputLayouts[0]);
	postEffectsPsoDesc.psoInputLayout.push_back(m_PostEffectsInputLayouts[1]);

	gfxDevice->CreatePipelineState(postEffectsPsoDesc, m_PostEffectsPSO, *m_PostEffectsRenderTarget.get());
	gfxDevice->CreateDescriptorSetLayout(m_PostEffectsSetLayout, m_PostEffectsInputLayouts[0].bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; ++i) {
		gfxDevice->CreateDescriptorSet(m_PostEffectsSetLayout, m_PostEffectsSet[i]);
		gfxDevice->WriteDescriptor(m_PostEffectsInputLayouts[0].bindings[0], m_PostEffectsSet[i], m_BloomRenderTarget->GetResolvedColorBuffer()[0]);
		gfxDevice->WriteDescriptor(m_PostEffectsInputLayouts[0].bindings[1], m_PostEffectsSet[i], m_PostProcessBuffer);
	}

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_GaussianBlurVertexShader, "../src/Samples/GaussianBlur/gaussian_blur_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_GaussianBlurFragmentShader, "../src/Samples/GaussianBlur/gaussian_blur_fragment.glsl");

	m_GaussianBlurInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(uint32_t) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }	// Input image to blur
		}
	};

	Graphics::PipelineStateDescription gaussianBlurPsoDesc = {};
	gaussianBlurPsoDesc.Name = "Gaussian Blur PSO";
	gaussianBlurPsoDesc.vertexShader = &m_GaussianBlurVertexShader;
	gaussianBlurPsoDesc.fragmentShader = &m_GaussianBlurFragmentShader;
	gaussianBlurPsoDesc.noVertex = true;
	gaussianBlurPsoDesc.cullMode = VK_CULL_MODE_NONE;
	gaussianBlurPsoDesc.psoInputLayout.push_back(m_GaussianBlurInputLayout);

	// Note: Assumption that we don't need to create two pipeline states.
	gfxDevice->CreatePipelineState(gaussianBlurPsoDesc, m_GaussianBlurPSO, *m_PingPongRenderTarget[0].get());
	gfxDevice->CreateDescriptorSetLayout(m_GaussianBlurSetLayout, m_GaussianBlurInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; ++i) {
		gfxDevice->CreateDescriptorSet(m_GaussianBlurSetLayout, m_GaussianBlurSet[i][0]);
		gfxDevice->CreateDescriptorSet(m_GaussianBlurSetLayout, m_GaussianBlurSet[i][1]);
		gfxDevice->CreateDescriptorSet(m_GaussianBlurSetLayout, m_GaussianBlurSet[i][2]);

		gfxDevice->WriteDescriptor(m_GaussianBlurInputLayout.bindings[0], m_GaussianBlurSet[i][0], m_PingPongRenderTarget[0]->GetColorBuffer());
		gfxDevice->WriteDescriptor(m_GaussianBlurInputLayout.bindings[0], m_GaussianBlurSet[i][1], m_PingPongRenderTarget[1]->GetColorBuffer());
		gfxDevice->WriteDescriptor(m_GaussianBlurInputLayout.bindings[0], m_GaussianBlurSet[i][2], m_BloomRenderTarget->GetResolvedColorBuffer()[1]);
	}
}

void GaussianBlur::CleanUp() {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_BloomRenderTarget.reset();

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(m_PSO);

	gfxDevice->DestroyShader(m_LightSourceVertexShader);
	gfxDevice->DestroyShader(m_LightSourceFragmentShader);
	gfxDevice->DestroyDescriptorSetLayout(m_LightSourceSetLayout);
	gfxDevice->DestroyPipeline(m_LightSourcePSO); 

	gfxDevice->DestroyShader(m_PostEffectsVertexShader);
	gfxDevice->DestroyShader(m_PostEffectsFragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_PostEffectsSetLayout);
	gfxDevice->DestroyPipeline(m_PostEffectsPSO); 

	gfxDevice->DestroyShader(m_GaussianBlurVertexShader);
	gfxDevice->DestroyShader(m_GaussianBlurFragmentShader);
	gfxDevice->DestroyDescriptorSetLayout(m_GaussianBlurSetLayout);
	gfxDevice->DestroyPipeline(m_GaussianBlurPSO);
}

void GaussianBlur::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	
	SCOPED_PROFILER_US("GaussianBlur::Update");

	m_Camera.OnUpdate(deltaT, input);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	SampleSceneData.Projection		= m_Camera.ProjectionMatrix;
	SampleSceneData.View			= m_Camera.ViewMatrix;
	SampleSceneData.TotalLights		= static_cast<uint32_t>(m_TotalLights);

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], &SampleSceneData);

	for (size_t ModelIndex = 0; ModelIndex < m_TotalModels; ++ModelIndex) {
		m_Models[ModelIndex]->OnUpdate(deltaT);
	}
	for (size_t LightIndex = 0; LightIndex < m_TotalLights; ++LightIndex) {
		Light& light = m_SceneLights[LightIndex];

		if (m_MovingLights) {
			float speed = 2.0f + (LightIndex % 3);
			float displacement = 3.5f;

			light.Position.z = glm::sin(constantT * speed) * displacement;
		}

		glm::mat4 toOrigin		= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f));
		glm::mat4 scale			= glm::scale(glm::mat4(1.0f), glm::vec3(m_LightScaleFactor));
		glm::mat4 toPosition	= glm::translate(glm::mat4(1.0f), glm::vec3(light.Position));

		light.Model = toPosition * scale * toOrigin;
	}

	gfxDevice->UpdateBuffer(m_LightBuffer, m_SceneLights.data());

	m_PostProcessingGPUData.Flags = ((m_BloomEnabled << 4) 
		| (m_HDRAcesEnabled << 3) 
		| (m_HDRExposureEnabled << 2) 
		| (m_GammaCorrectionEnabled << 1) 
		| (m_HDRReinhardEnabled));

	m_PostProcessingGPUData.Gamma		= m_Gamma;
	m_PostProcessingGPUData.HDRExposure = m_HDRExposure;

	gfxDevice->UpdateBuffer(m_PostProcessBuffer, &m_PostProcessingGPUData);
}

void GaussianBlur::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("GaussianBlur::RenderScene");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();


	{
		SCOPED_PROFILER_US("GaussianBlur::GeometryPass");
		m_BloomRenderTarget->Begin(commandBuffer);
		
		RenderSceneGeometry(currentFrame, commandBuffer);
		RenderLightSources(currentFrame, commandBuffer);

		m_BloomRenderTarget->End(commandBuffer);
	}

	if (m_BloomEnabled) {
		SCOPED_PROFILER_US("GaussianBlur::GaussianBlur")
		GaussianBlurPass(currentFrame, commandBuffer);
	}

	{
		SCOPED_PROFILER_US("GaussianBlur::PostEffectsPass");

		m_PostEffectsRenderTarget->Begin(commandBuffer);
		
		RenderPostEffects(currentFrame, commandBuffer);

		m_PostEffectsRenderTarget->End(commandBuffer);
	}

	{
		SCOPED_PROFILER_US("GaussianBlur::Copying to SwapChain");
		m_PostEffectsRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_PostEffectsRenderTarget->GetColorBuffer());
	}
}

void GaussianBlur::RenderSceneGeometry(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	SCOPED_PROFILER_US("GaussianBlur::RenderSceneGeometry");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	gfxDevice->BindDescriptorSet(m_Set[currentFrame], commandBuffer, m_PSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);

	for (int ModelIndex = 0; ModelIndex < m_TotalModels; ++ModelIndex) {

		Assets::Model& Model = *m_Models[ModelIndex].get();

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model.TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Model.DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, Model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		SamplePushConstants.Model = Model.GetModelMatrix();

		for (const auto& Mesh: Model.Meshes) {

			SamplePushConstants.MaterialIndex = static_cast<uint32_t>(Mesh.MaterialIndex);
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
}

void GaussianBlur::RenderLightSources(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("GaussianBlur::RenderLightSources");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->BindDescriptorSet(m_LightSourceDescriptorSet[currentFrame], commandBuffer, m_LightSourcePSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightSourcePSO.pipeline);

	for (size_t LightIndex = 0; LightIndex < m_TotalLights; ++LightIndex) {
		LightSourcePushConstants.Model = m_SceneLights[LightIndex].Model;
		LightSourcePushConstants.Color = m_SceneLights[LightIndex].Color;

		vkCmdPushConstants(commandBuffer, m_LightSourcePSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(LightSourceRenderPushConstants), &LightSourcePushConstants);
		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}
}

void GaussianBlur::RenderPostEffects(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("GaussianBlur::RenderPostEffects");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->BindDescriptorSet(m_PostEffectsSet[currentFrame], commandBuffer, m_PostEffectsPSO.pipelineLayout, 0, 1);
	gfxDevice->BindDescriptorSet(m_GaussianBlurSet[currentFrame][!m_HorizontalBlurPass], commandBuffer, m_PostEffectsPSO.pipelineLayout, 1, 1);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostEffectsPSO.pipeline);

	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void GaussianBlur::GaussianBlurPass(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	SCOPED_PROFILER_US("GaussianBlur::GaussianBlurPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_HorizontalBlurPass = true;
	bool firstIteration = true;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GaussianBlurPSO.pipeline);

	for (uint32_t blurPassIndex = 0; blurPassIndex < m_BlurPassCount; ++blurPassIndex) {
		m_PingPongRenderTarget[m_HorizontalBlurPass]->Begin(commandBuffer);

		const uint32_t pushConstantValue = m_HorizontalBlurPass;

		if (firstIteration) {
			gfxDevice->BindDescriptorSet(m_GaussianBlurSet[currentFrame][2], commandBuffer, m_GaussianBlurPSO.pipelineLayout, 0, 1);
			firstIteration = false;
		}
		else {
			gfxDevice->BindDescriptorSet(m_GaussianBlurSet[currentFrame][!m_HorizontalBlurPass], commandBuffer, m_GaussianBlurPSO.pipelineLayout, 0, 1);
		}

		vkCmdPushConstants(commandBuffer, m_GaussianBlurPSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(uint32_t), &pushConstantValue);
		vkCmdDraw(commandBuffer, 6, 1, 0, 0);

		m_PingPongRenderTarget[m_HorizontalBlurPass]->End(commandBuffer);
		
		m_HorizontalBlurPass = !m_HorizontalBlurPass;
	}
}

void GaussianBlur::RenderUI() {
	SCOPED_PROFILER_US("GaussianBlur::UI");
	ImGui::SeparatorText("Scene Settings");

	m_Camera.OnUIRender("Main Camera - Settings");

	ImGui::Checkbox("Gamma Correction", &m_GammaCorrectionEnabled);

	if (m_GammaCorrectionEnabled) {
		ImGui::DragFloat("Gamma", &m_Gamma, 0.02f, 0.0f, 10.0f);
	}

	ImGui::Checkbox("HDR - Reinhard Tone Mapping", &m_HDRReinhardEnabled);

	if (m_HDRReinhardEnabled) {
		m_HDRExposureEnabled = false;
		m_HDRAcesEnabled = false;
	}

	ImGui::Checkbox("HDR - Exposure Tone Mapping", &m_HDRExposureEnabled);
	
	if (m_HDRExposureEnabled) {
		ImGui::DragFloat("HDR Exposure", &m_HDRExposure, 0.02f, 0.0f, 10.0f);
		m_HDRReinhardEnabled = false;
		m_HDRAcesEnabled = false;
	}

	ImGui::Checkbox("HDR - ACES Cinematic Tone Mapping", &m_HDRAcesEnabled);
	
	if (m_HDRAcesEnabled) {
		m_HDRExposureEnabled = false;
		m_HDRReinhardEnabled = false;
	}

	ImGui::Checkbox("Moving Lights", &m_MovingLights);
	ImGui::Checkbox("Bloom", &m_BloomEnabled);
	ImGui::DragFloat("Bloom Threshold", &SampleSceneData.BloomThreshold, 0.02f, 0.0f, 2.0f);
	ImGui::DragInt("Blur Pass Count", reinterpret_cast<int*>(&m_BlurPassCount), 1.0f, 0, 20);
	ImGui::DragFloat("Light Scale Factor", &m_LightScaleFactor, 0.02f, 0.0f, 2.0f);
	ImGui::DragFloat("Light Position Offset", &m_LightPositionOffset, 0.02f, -10.0f, 10.0f);

	if (ImGui::Button("Add Light")) {
		AddLight();
	}
	
	if (ImGui::Button("Remove Light")) {
		RemoveLight();
	}

	if (ImGui::TreeNode("Lights")) {
		for (size_t LightIndex = 0; LightIndex < m_TotalLights; ++LightIndex) {
			std::string light_id = "light_" + std::to_string(LightIndex);

			if (ImGui::TreeNode(light_id.c_str())) {
				ImGui::DragFloat4("Position",		(float*)&m_SceneLights[LightIndex].Position, 0.02f, -40.0f, 40.0f);
				ImGui::ColorPicker4("Color",		(float*)&m_SceneLights[LightIndex].Color);
				ImGui::DragFloat("Light Intensity", (float*)&m_SceneLights[LightIndex].Color.a, 0.02f, 0.0f, 1.0f);
				ImGui::DragFloat("Light Linear",	(float*)&m_SceneLights[LightIndex].Linear, 0.02f, -20.0f, 20.0f);
				ImGui::DragFloat("Light Quadratic", (float*)&m_SceneLights[LightIndex].Quadratic, 0.02f, -20.0f, 20.0f);

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	for (int ModelIndex = 0; ModelIndex < m_TotalModels; ++ModelIndex) {
		m_Models[ModelIndex]->OnUIRender();
	}
}

void GaussianBlur::Resize(uint32_t width, uint32_t height) {
	m_ScreenWidth	= width;
	m_ScreenHeight	= height;

	m_Camera.Resize(m_ScreenWidth, m_ScreenHeight);
	m_BloomRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
	m_PostEffectsRenderTarget->Resize(m_ScreenWidth, m_ScreenHeight);
	m_PingPongRenderTarget[0]->Resize(m_ScreenWidth, m_ScreenHeight);
	m_PingPongRenderTarget[1]->Resize(m_ScreenWidth, m_ScreenHeight);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; ++i) {
		gfxDevice->WriteDescriptor(m_PostEffectsInputLayouts[0].bindings[0], m_PostEffectsSet[i], m_BloomRenderTarget->GetResolvedColorBuffer()[0]);
		
		gfxDevice->WriteDescriptor(m_GaussianBlurInputLayout.bindings[0], m_GaussianBlurSet[i][0], m_PingPongRenderTarget[0]->GetColorBuffer());
		gfxDevice->WriteDescriptor(m_GaussianBlurInputLayout.bindings[0], m_GaussianBlurSet[i][1], m_PingPongRenderTarget[1]->GetColorBuffer());
		gfxDevice->WriteDescriptor(m_GaussianBlurInputLayout.bindings[0], m_GaussianBlurSet[i][2], m_BloomRenderTarget->GetResolvedColorBuffer()[1]);
	}
}

RUN_APPLICATION(GaussianBlur);