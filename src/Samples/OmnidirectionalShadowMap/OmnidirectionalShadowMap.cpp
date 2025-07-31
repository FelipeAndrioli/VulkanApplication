#include <memory>

#include "../Core/Application.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/Settings.h"
#include "../Core/RenderTarget.h"
#include "../Core/ResourceManager.h"

#include "../Core/Profiler.h"

#include "../Assets/Model.h"
#include "../Assets/Camera.h"
#include "../Assets/ShadowCamera.h"

#include "../Utils/ModelLoader.h"

constexpr int MAX_MODELS = 5;

/*
	Notes:
		
		-	I can use the same descriptor binding location to send the shadow map intended to generate the shadows, however 
			the descriptors cannot be updated while binded so I cannot update all descriptors at once. There is a 'technique'
			to handle this scenario but I'll implement it later, for now the double binding location with branching in the
			shader will do it.

		-	One single image can have multiple layouts, so I'm creating two layouts for the multiple framebuffer images, one
			per cube face to use as framebuffer attachment, and other with 6 layers to use as the shader input. The single
			framebuffer image contains a single layout since I'll use it as framebuffer attachment and shader input.
*/

class OmnidirectionalShadowMap : public Application::IScene {
public:
	OmnidirectionalShadowMap() {
		settings.Title = "Omnidirectional Shadow Map.exe";
		settings.Width = 1600;
		settings.Height = 800;
		settings.uiEnabled = true;

		m_Width = settings.Width;
		m_Height = settings.Height;
	}

	virtual void StartUp()																		override;
	virtual void CleanUp()																		override;
	virtual void Update(const float ConstantT, const float DeltaT, InputSystem::Input& Input)	override;
	virtual void RenderScene(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer) override;
	virtual void RenderUI()																		override;
	virtual void Resize(uint32_t Width, uint32_t Height)										override;

private:
	// 256 bytes
	struct GPUData {
		int extra0 = 0;
		int extra1 = 0;

		uint32_t Flags				= 0;				// 4 -> 0 - Shadow Map Enabled | 1 - PCF Enabled | 2 - Optimized PCF Enabled  | 3 - Geometry Shader
		uint32_t LightFarDistance	= 0;

		glm::vec4 extra			= {};					// 16
		glm::vec4 ViewPosition	= {};					// 16
		glm::vec4 LightPosition = glm::vec4(0.0f);		// 16

		glm::mat4 View				= glm::mat4(1.0f);	// 64
		glm::mat4 Projection		= glm::mat4(1.0f);	// 64
		glm::mat4 LightProjection	= glm::mat4(1.0f);	// 64
	} m_SceneGPUData;

	// 256
	struct ModelGPUData {
		glm::vec4 extra[12] = {};
		glm::mat4 Model		= glm::mat4(1.0f);	// 64
	} m_ModelGPUData[MAX_MODELS];

	// Scene
	std::shared_ptr<Assets::Model> m_Models[MAX_MODELS];

	uint32_t m_TotalModels	= 0;
	uint32_t m_Width		= 0;
	uint32_t m_Height		= 0;

	bool m_SingleFramebufferPass	= true;
	bool m_RotateLight				= true;
	bool m_ShadowMapEnabled			= true;
	bool m_PCFEnabled				= false;
	bool m_OptimizedPCFEnabled		= false;

	glm::vec4 m_LightPosition = glm::vec4(0.0f, 2.0f, 0.0f, 0.0f);

	const glm::vec3 m_CameraInitialPosition = glm::vec3(-2.2f, 4.0f, -12.0f);

	const float m_CameraInitialFov			= 45.0f;
	const float m_CameraInitialYaw			= 73.0f;
	const float m_CameraInitialPitch		= -19.0f;

	Assets::Camera m_Camera = {};
	Assets::ShadowCamera m_ShadowCamera = {};

	// Shadow Rendering
	std::unique_ptr<Graphics::DepthOnlyCubeRenderTarget> m_OmniDirectionalRenderTarget;
	Graphics::InputLayout				m_ShadowInputLayout								= {};
	Graphics::PipelineState				m_ShadowPSO										= {};
	Graphics::PipelineStateDescription	m_ShadowPSODescription							= {};
	Graphics::Shader					m_ShadowVertexShader							= {};
	Graphics::Shader					m_ShadowFragmentShader							= {};

	VkDescriptorSet						m_ShadowDescriptor[Graphics::FRAMES_IN_FLIGHT]	= {};

	const float m_InitialPointLightNear = 0.1f;
	const float m_InitialPointLightFar	= 100.0f;

	// Shadow Rendering (Single Framebuffer - Geometry Shader)
	std::unique_ptr<Graphics::DepthOnlyCubeRenderTarget> m_OmniDirectionalSingleFramebufferRenderTarget;
	Graphics::InputLayout				m_ShadowSingleFramebufferInputLayout	= {};
	Graphics::PipelineState				m_ShadowSingleFramebufferPSO			= {};
	Graphics::PipelineStateDescription	m_ShadowSingleFramebufferPSODescription = {};
	Graphics::Shader					m_ShadowSingleFramebufferVertexShader	= {};
	Graphics::Shader					m_ShadowSingleFramebufferGeometryShader = {};
	Graphics::Shader					m_ShadowSingleFramebufferFragmentShader = {};

	Graphics::GPUBuffer					m_StorageBuffer[Graphics::FRAMES_IN_FLIGHT] = {};

	VkDescriptorSet						m_ShadowSingleFramebufferDescriptor[Graphics::FRAMES_IN_FLIGHT] = {};

	// 80 bytes
	struct ShadowPushConstants {
		int extra1;							// 4
		int extra2;							// 4
		int extra3;							// 4
		int ModelIndex;						// 4
		glm::mat4 View = glm::mat4(1.0f);	// 64
	} m_ShadowPushConstants = {};

	// Scene Rendering
	std::unique_ptr<Graphics::OffscreenRenderTarget> m_SceneRenderTarget;
	Graphics::InputLayout				m_SceneInputLayout								= {};
	Graphics::PipelineState				m_ScenePSO										= {};
	Graphics::PipelineStateDescription	m_ScenePSODescription							= {};
	Graphics::Shader					m_SceneVertexShader								= {};
	Graphics::Shader					m_SceneFragmentShader							= {};
	Graphics::Buffer					m_SceneBuffer[Graphics::FRAMES_IN_FLIGHT]		= {};
	Graphics::Buffer					m_ModelsBuffer[Graphics::FRAMES_IN_FLIGHT]		= {};	// Buffer to hold Models Model Matrix

	VkDescriptorSet						m_SceneDescriptor[Graphics::FRAMES_IN_FLIGHT]	= {};

private:
	void ColorPassStartUp();
	void ShadowPassStartUp();
	void ShadowSingleFramebufferPassStartUp();

	void ColorPass(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer, const std::shared_ptr<Assets::Model> Models[MAX_MODELS], const uint32_t ModelsCount);
	void ShadowPass(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer, const std::shared_ptr<Assets::Model> Models[MAX_MODELS], const uint32_t ModelsCount);
	void ShadowSingleFramebufferPass(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer, const std::shared_ptr<Assets::Model> Models[MAX_MODELS], const uint32_t ModelsCount);
};

void OmnidirectionalShadowMap::ColorPassStartUp() {

	m_SceneInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(int) }		// Model Index
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },			// GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },			// Model GPU Data
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },	// Cube Shadow Map (Multi Framebuffer)
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }	// Cube Shadow Map (Single Framebuffer)
		}
	};

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_SceneVertexShader,	"../src/Samples/OmnidirectionalShadowMap/scene_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SceneFragmentShader,	"../src/Samples/OmnidirectionalShadowMap/scene_fragment.glsl");

	m_ScenePSODescription.Name				= "Scene PSO";
	m_ScenePSODescription.vertexShader		= &m_SceneVertexShader;
	m_ScenePSODescription.fragmentShader	= &m_SceneFragmentShader;
	m_ScenePSODescription.cullMode			= VK_CULL_MODE_BACK_BIT;
	m_ScenePSODescription.psoInputLayout.push_back(m_SceneInputLayout);

	gfxDevice->CreatePipelineState(m_ScenePSODescription, m_ScenePSO, *m_SceneRenderTarget.get());

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_ScenePSO.descriptorSetLayout, m_SceneDescriptor[i]);
		gfxDevice->WriteDescriptor(m_SceneInputLayout.bindings[0], m_SceneDescriptor[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(m_SceneInputLayout.bindings[1], m_SceneDescriptor[i], m_ModelsBuffer[i]);
		gfxDevice->WriteDescriptor(m_SceneInputLayout.bindings[2], m_SceneDescriptor[i], m_OmniDirectionalRenderTarget->GetDepthBuffer());
		gfxDevice->WriteDescriptor(m_SceneInputLayout.bindings[3], m_SceneDescriptor[i], m_OmniDirectionalSingleFramebufferRenderTarget->GetDepthBuffer());
	}
}

void OmnidirectionalShadowMap::ShadowPassStartUp() {

	m_ShadowInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(ShadowPushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },		// GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }			// Models GPU Data
		}
	};

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_ShadowVertexShader,	"../src/Samples/OmnidirectionalShadowMap/omni_shadow_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_ShadowFragmentShader, "../src/Samples/OmnidirectionalShadowMap/omni_shadow_fragment.glsl");

	m_ShadowPSODescription.Name				= "Omnidirectional Shadow Mapping PSO";
	m_ShadowPSODescription.vertexShader		= &m_ShadowVertexShader;
	m_ShadowPSODescription.fragmentShader	= &m_ShadowFragmentShader;
	m_ShadowPSODescription.cullMode			= VK_CULL_MODE_NONE;
	m_ShadowPSODescription.psoInputLayout.push_back(m_ShadowInputLayout);

	gfxDevice->CreatePipelineState(m_ShadowPSODescription, m_ShadowPSO, *m_OmniDirectionalRenderTarget.get());

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_ShadowPSO.descriptorSetLayout, m_ShadowDescriptor[i]);
		gfxDevice->WriteDescriptor(m_ShadowInputLayout.bindings[0], m_ShadowDescriptor[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(m_ShadowInputLayout.bindings[1], m_ShadowDescriptor[i], m_ModelsBuffer[i]);
	}
}

void OmnidirectionalShadowMap::ShadowSingleFramebufferPassStartUp() {

	m_ShadowSingleFramebufferInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(ShadowPushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT },		// GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },									// Models GPU Data
			{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_GEOMETRY_BIT }									// View Matrices
		}
	};

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_ShadowSingleFramebufferVertexShader,		"../src/Samples/OmnidirectionalShadowMap/omni_shadow_sf_vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_GEOMETRY_BIT,	m_ShadowSingleFramebufferGeometryShader,	"../src/Samples/OmnidirectionalShadowMap/omni_shadow_sf_geometry.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_ShadowSingleFramebufferFragmentShader,	"../src/Samples/OmnidirectionalShadowMap/omni_shadow_sf_fragment.glsl");

	m_ShadowSingleFramebufferPSODescription.Name				= "Omnidirectional Single Framebuffer Shadow Mapping PSO";
	m_ShadowSingleFramebufferPSODescription.vertexShader		= &m_ShadowSingleFramebufferVertexShader;
	m_ShadowSingleFramebufferPSODescription.geometryShader		= &m_ShadowSingleFramebufferGeometryShader;
	m_ShadowSingleFramebufferPSODescription.fragmentShader		= &m_ShadowSingleFramebufferFragmentShader;
	m_ShadowSingleFramebufferPSODescription.cullMode			= VK_CULL_MODE_NONE;
	m_ShadowSingleFramebufferPSODescription.psoInputLayout.push_back(m_ShadowSingleFramebufferInputLayout);

	gfxDevice->CreatePipelineState(m_ShadowSingleFramebufferPSODescription, m_ShadowSingleFramebufferPSO, *m_OmniDirectionalSingleFramebufferRenderTarget.get());

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_ShadowSingleFramebufferPSO.descriptorSetLayout, m_ShadowSingleFramebufferDescriptor[i]);
		gfxDevice->WriteDescriptor(m_ShadowSingleFramebufferInputLayout.bindings[0], m_ShadowSingleFramebufferDescriptor[i], m_SceneBuffer[i]);
		gfxDevice->WriteDescriptor(m_ShadowSingleFramebufferInputLayout.bindings[1], m_ShadowSingleFramebufferDescriptor[i], m_ModelsBuffer[i]);
		gfxDevice->WriteDescriptor(m_ShadowSingleFramebufferInputLayout.bindings[2], m_ShadowSingleFramebufferDescriptor[i], m_StorageBuffer[i]);
	}
}

void OmnidirectionalShadowMap::StartUp() {
	m_Camera.Init(m_CameraInitialPosition, m_CameraInitialFov, m_CameraInitialYaw, m_CameraInitialPitch, m_Width, m_Height);
	m_ShadowCamera.Init(m_LightPosition, 0, 0, 0, m_Width, m_Height);
	m_ShadowCamera.SetPointLightSettings(m_InitialPointLightNear, m_InitialPointLightFar);

	m_SceneRenderTarget								= std::make_unique<Graphics::OffscreenRenderTarget>(m_Width, m_Height);
	m_OmniDirectionalRenderTarget					= std::make_unique<Graphics::DepthOnlyCubeRenderTarget>(m_Width, m_Height, 0, 0, false);
	m_OmniDirectionalSingleFramebufferRenderTarget	= std::make_unique<Graphics::DepthOnlyCubeRenderTarget>(m_Width, m_Height, 0, 6, true);

	m_Models[m_TotalModels] = ModelLoader::LoadModel(ModelType::QUAD);
	m_Models[m_TotalModels]->Transformations.rotation.x = 90.0f;
	m_Models[m_TotalModels]->Transformations.scaleHandler = 20.0f;
	m_Models[m_TotalModels]->ModelIndex = m_TotalModels;

	m_TotalModels++;

	m_Models[m_TotalModels] = ModelLoader::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf");
	m_Models[m_TotalModels]->Transformations.scaleHandler	= 18.0f;
	m_Models[m_TotalModels]->Transformations.translation	= glm::vec3(0.0f, 1.5f, 0.0f);
	m_Models[m_TotalModels]->Transformations.rotation		= glm::vec3(0.0f, 0.0f, 0.0f);
	m_Models[m_TotalModels]->ModelIndex = m_TotalModels;

	m_TotalModels++;

	m_Models[m_TotalModels] = ModelLoader::LoadModel(ModelType::CUBE);
	m_Models[m_TotalModels]->Transformations.scaleHandler = 1.0f;
	m_Models[m_TotalModels]->Transformations.translation.x = 4.0f;
	m_Models[m_TotalModels]->Transformations.translation.y = 0.5f;
	m_Models[m_TotalModels]->ModelIndex = m_TotalModels;

	m_TotalModels++;

	m_Models[m_TotalModels] = ModelLoader::LoadModel(ModelType::CUBE);
	m_Models[m_TotalModels]->Transformations.scaleHandler = 1.0f;
	m_Models[m_TotalModels]->Transformations.translation.x = -4.0f;
	m_Models[m_TotalModels]->Transformations.translation.y = 0.5f;
	m_Models[m_TotalModels]->ModelIndex = m_TotalModels;

	m_TotalModels++;
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		m_StorageBuffer[i]	= gfxDevice->CreateStorageBuffer(6 * sizeof(glm::mat4));
		m_SceneBuffer[i]	= gfxDevice->CreateBuffer(sizeof(GPUData));						// Buffer suballocation, will be automatically destroyed
		m_ModelsBuffer[i]	= gfxDevice->CreateBuffer(sizeof(ModelGPUData) * MAX_MODELS);	// Buffer suballocation, will be automatically destroyed
	}

	ShadowPassStartUp();
	ShadowSingleFramebufferPassStartUp();
	ColorPassStartUp();
}

void OmnidirectionalShadowMap::CleanUp() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyShader(m_SceneVertexShader);
	gfxDevice->DestroyShader(m_SceneFragmentShader);
	gfxDevice->DestroyShader(m_ShadowVertexShader);
	gfxDevice->DestroyShader(m_ShadowFragmentShader);
	gfxDevice->DestroyShader(m_ShadowSingleFramebufferVertexShader);
	gfxDevice->DestroyShader(m_ShadowSingleFramebufferGeometryShader);
	gfxDevice->DestroyShader(m_ShadowSingleFramebufferFragmentShader);

	gfxDevice->DestroyPipeline(m_ScenePSO);
	gfxDevice->DestroyPipeline(m_ShadowPSO);
	gfxDevice->DestroyPipeline(m_ShadowSingleFramebufferPSO);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->DestroyBuffer(m_StorageBuffer[i]);
	}

	for (uint32_t i = 0; i < m_TotalModels; i++) {
		m_Models[i].reset();
	}
}

void OmnidirectionalShadowMap::Update(const float ConstantT, const float DeltaT, InputSystem::Input& Input) {
	SCOPED_PROFILER_US("OmnidirectionalShadowMap::Update");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (uint32_t CurrentModelIndex = 0; CurrentModelIndex < m_TotalModels; CurrentModelIndex++) {
		m_Models[CurrentModelIndex]->OnUpdate(DeltaT);
		m_ModelGPUData[CurrentModelIndex].Model = m_Models[CurrentModelIndex]->GetModelMatrix();
	}

	if (m_RotateLight) {
		m_LightPosition.x = glm::sin(ConstantT + 5.0f) * 7.0f;
		m_LightPosition.z = glm::cos(ConstantT + 5.0f) * 7.0f;
	}

	m_Camera.OnUpdate(DeltaT, Input);
	m_ShadowCamera.UpdateOmniDirectionalShadowMatrix(m_LightPosition);

	m_SceneGPUData.View				= m_Camera.ViewMatrix;
	m_SceneGPUData.Projection		= m_Camera.ProjectionMatrix;
	m_SceneGPUData.LightProjection	= m_ShadowCamera.ProjectionMatrix;
	m_SceneGPUData.LightFarDistance = m_ShadowCamera.PointLightFar;
	m_SceneGPUData.LightPosition	= m_LightPosition;
	m_SceneGPUData.ViewPosition		= glm::vec4(m_Camera.Position, 1.0f);
	m_SceneGPUData.Flags			= ((m_SingleFramebufferPass << 3) | (m_OptimizedPCFEnabled << 2) | (m_PCFEnabled << 1) | (m_ShadowMapEnabled));

	gfxDevice->UpdateBuffer(m_SceneBuffer[gfxDevice->GetCurrentFrameIndex()], &m_SceneGPUData);
	gfxDevice->UpdateBuffer(m_ModelsBuffer[gfxDevice->GetCurrentFrameIndex()], &m_ModelGPUData);

	if (m_SingleFramebufferPass) {
		gfxDevice->UpdateBuffer(m_StorageBuffer[gfxDevice->GetCurrentFrameIndex()], 0, &m_ShadowCamera.OmniViewMatrix, 6 * sizeof(glm::mat4));
	}
}

void OmnidirectionalShadowMap::ColorPass(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer, const std::shared_ptr<Assets::Model> Models[MAX_MODELS], const uint32_t ModelsCount) {
	SCOPED_PROFILER_US("OmnidirectionalShadowMap::ColorPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_SceneRenderTarget->Begin(CommandBuffer);

	gfxDevice->BindDescriptorSet(m_SceneDescriptor[CurrentFrame], CommandBuffer, m_ScenePSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ScenePSO.pipeline);

	for (uint32_t ModelIndex = 0; ModelIndex < ModelsCount; ModelIndex++) {
		const std::shared_ptr<Assets::Model> Model = Models[ModelIndex];

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model->TotalIndices };
		
		vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &Model->DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(CommandBuffer, Model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdPushConstants(CommandBuffer, m_ScenePSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(int), &Model->ModelIndex);

		for (uint32_t MeshIndex = 0; MeshIndex < Model->Meshes.size(); MeshIndex++) {
			Assets::Mesh& Mesh = Model->Meshes[MeshIndex];

			vkCmdDrawIndexed(CommandBuffer, Mesh.Indices.size(), 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);
		}
	}

	m_SceneRenderTarget->End(CommandBuffer);
}

void OmnidirectionalShadowMap::ShadowPass(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer, const std::shared_ptr<Assets::Model> Models[MAX_MODELS], const uint32_t ModelsCount) {
	SCOPED_PROFILER_US("OmnidirectionalShadowMap::ShadowPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (uint32_t LightFaceIndex = 0; LightFaceIndex < 6; LightFaceIndex++) {
		m_OmniDirectionalRenderTarget->Begin(CommandBuffer, LightFaceIndex);

		gfxDevice->BindDescriptorSet(m_ShadowDescriptor[CurrentFrame], CommandBuffer, m_ShadowPSO.pipelineLayout, 0, 1);
		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowPSO.pipeline);

		for (uint32_t ModelIndex = 0; ModelIndex < m_TotalModels; ModelIndex++) {
			const std::shared_ptr<Assets::Model> Model = m_Models[ModelIndex];

			VkDeviceSize offsets[] = { sizeof(uint32_t) * Model->TotalIndices };
			
			vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &Model->DataBuffer.Handle, offsets);
			vkCmdBindIndexBuffer(CommandBuffer, Model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

			m_ShadowPushConstants.ModelIndex	= Model->ModelIndex;
			m_ShadowPushConstants.View			= m_ShadowCamera.OmniViewMatrix[LightFaceIndex];

			vkCmdPushConstants(CommandBuffer, m_ShadowPSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(ShadowPushConstants), &m_ShadowPushConstants);

			for (uint32_t MeshIndex = 0; MeshIndex < Model->Meshes.size(); MeshIndex++) {
				Assets::Mesh& Mesh = Model->Meshes[MeshIndex];

				vkCmdDrawIndexed(CommandBuffer, Mesh.Indices.size(), 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);
			}
		}	

		m_OmniDirectionalRenderTarget->End(CommandBuffer);
	}
}

void OmnidirectionalShadowMap::ShadowSingleFramebufferPass(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer, const std::shared_ptr<Assets::Model> Models[MAX_MODELS], const uint32_t ModelsCount) {
	SCOPED_PROFILER_US("OmnidirectionalShadowMap::ShadowSingleFramebufferPass");

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OmniDirectionalSingleFramebufferRenderTarget->Begin(CommandBuffer, 0);

	gfxDevice->BindDescriptorSet(m_ShadowSingleFramebufferDescriptor[CurrentFrame], CommandBuffer, m_ShadowSingleFramebufferPSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowSingleFramebufferPSO.pipeline);

	for (uint32_t ModelIndex = 0; ModelIndex < ModelsCount; ModelIndex++) {
		const std::shared_ptr<Assets::Model> Model = Models[ModelIndex];

		VkDeviceSize offsets[] = { sizeof(uint32_t) * Model->TotalIndices };
		
		vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &Model->DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(CommandBuffer, Model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdPushConstants(CommandBuffer, m_ShadowSingleFramebufferPSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(int), &Model->ModelIndex);

		for (uint32_t MeshIndex = 0; MeshIndex < Model->Meshes.size(); MeshIndex++) {
			Assets::Mesh& Mesh = Model->Meshes[MeshIndex];

			vkCmdDrawIndexed(CommandBuffer, Mesh.Indices.size(), 1, Mesh.IndexOffset, Mesh.VertexOffset, 0);
		}
	}

	m_OmniDirectionalSingleFramebufferRenderTarget->End(CommandBuffer);
}

void OmnidirectionalShadowMap::RenderScene(const uint32_t CurrentFrame, const VkCommandBuffer& CommandBuffer) {
	SCOPED_PROFILER_US("OmnidirectionalShadowMap::RenderScene");

	if (m_ShadowMapEnabled && !m_SingleFramebufferPass)
		ShadowPass(CurrentFrame, CommandBuffer, m_Models, m_TotalModels);
	
	if (m_ShadowMapEnabled && m_SingleFramebufferPass)
		ShadowSingleFramebufferPass(CurrentFrame, CommandBuffer, m_Models, m_TotalModels);

	ColorPass(CurrentFrame, CommandBuffer, m_Models, m_TotalModels);

	{
		// TODO: Implement a better solution to move offscreen render result to the swapchain framebuffer as this approach takes most of the rendering time.

		SCOPED_PROFILER_US("Layout change and copy");
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
		m_SceneRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_SceneRenderTarget->GetColorBuffer());
	}

}

void OmnidirectionalShadowMap::RenderUI() {

	m_Camera.OnUIRender("Main Camera Settings");
	m_ShadowCamera.OnUIRender("Shadow Camera Settings");

	for (uint32_t ModelIndex = 0; ModelIndex < m_TotalModels; ModelIndex++) {
		m_Models[ModelIndex]->OnUIRender();
	}

	ImGui::Separator();
	ImGui::Checkbox("Shadow Map Enabled",				&m_ShadowMapEnabled);
	ImGui::Checkbox("Shadow Map PCF Enabled",			&m_PCFEnabled);
	ImGui::Checkbox("Shadow Map Optimized PCF Enabled", &m_OptimizedPCFEnabled);
	ImGui::Checkbox("Rotate Light", &m_RotateLight);
	ImGui::DragFloat4("Light Position", (float*)&m_LightPosition, 0.002, -10.0f, 10.0f);
	
	ImGui::Separator();
	ImGui::Checkbox("Single Framebuffer Pass (Geometry Shader)", &m_SingleFramebufferPass);
}

void OmnidirectionalShadowMap::Resize(uint32_t Width, uint32_t Height) {
	m_Width		= Width;
	m_Height	= Height;

	m_Camera.Resize(m_Width, m_Height);
	m_ShadowCamera.Resize(m_Width, m_Height);

	m_OmniDirectionalRenderTarget->Resize(m_Width, m_Height);
	m_OmniDirectionalSingleFramebufferRenderTarget->Resize(m_Width, m_Height);
	m_SceneRenderTarget->Resize(m_Width, m_Height);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (uint32_t FrameIndex = 0; FrameIndex < Graphics::FRAMES_IN_FLIGHT; ++FrameIndex) {
		gfxDevice->WriteDescriptor(m_SceneInputLayout.bindings[2], m_SceneDescriptor[FrameIndex], m_OmniDirectionalRenderTarget->GetDepthBuffer());
		gfxDevice->WriteDescriptor(m_SceneInputLayout.bindings[3], m_SceneDescriptor[FrameIndex], m_OmniDirectionalSingleFramebufferRenderTarget->GetDepthBuffer());
	}
}

RUN_APPLICATION(OmnidirectionalShadowMap);
