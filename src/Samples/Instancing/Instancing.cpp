#include <memory>

#include "../Core/Application.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/Settings.h"
#include "../Core/RenderTarget.h"
#include "../Core/ResourceManager.h"

#include "../Assets/Camera.h"
#include "../Assets/Model.h"

#include "../Utils/ModelLoader.h"

#define MAX_MODELS 5000

class Instancing : public Application::IScene {
public:
	Instancing() {
		settings.Title			= "Instancing.exe";
		settings.Width			= 1600;
		settings.Height			= 800;
		settings.uiEnabled		= true;

		m_Width					= settings.Width;
		m_Height				= settings.Height;
	}

	virtual void StartUp()																		override;
	virtual void CleanUp()																		override;
	virtual void Update(const float constantT, const float deltaT, InputSystem::Input& input)	override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) override;
	virtual void RenderUI()																		override;
	virtual void Resize(uint32_t width, uint32_t height)										override;
private:

	struct PushConstant {
		int materialIndex;
	} m_PushConstant;

	struct SceneGPUData {
		float time				= 0.0f;								// 4
		float lightIntensity	= 1.0f;								// 8
		int extra_2				= 0;								// 12
		int extra_3				= 0;								// 16
		glm::vec4 lightPos		= glm::vec4(0.0, 4.4, 3.2, 0.0);	// 32
		glm::vec4 lightColor	= glm::vec4(1.0);					// 48
		glm::vec4 extra[5]		= {};								// 128
		glm::mat4 view			= glm::mat4(1.0f);					// 192
		glm::mat4 proj			= glm::mat4(1.0f);					// 256
	} m_SceneGPUData;

	struct ModelGPUData {
		int flip_uv_vertically	= 0;				// 4
		int extra_1				= 0;				// 8
		int extra_2				= 0;				// 12
		int extra_3				= 0;				// 16
		glm::vec4 extra[7]		= {};				// 128
		glm::mat4 model			= glm::mat4(1.0f);	// 192
		glm::mat4 normal		= glm::mat4(1.0f);	// 256
	} m_ModelGPUData;

	struct InstanceModelData {
		glm::mat4 model;
	};

	std::vector<InstanceModelData> m_InstanceModelData;

	std::shared_ptr<Assets::Model>	m_Model;

	uint32_t	m_Width				= 0;
	uint32_t	m_Height			= 0;
	uint32_t	m_DrawCalls			= 0;
	uint32_t	m_TotalVertices		= 0;
	uint32_t	m_ModelVertices		= 0;

	bool m_FirstPass				= true;

	Assets::Camera	m_Camera		= {};

	Graphics::GPUBuffer			m_StorageBuffer									= {};
	Graphics::Buffer			m_SceneDataBuffer[Graphics::FRAMES_IN_FLIGHT]	= {};
	Graphics::Buffer			m_ModelDataBuffer								= {};
	Graphics::Shader			m_VertexShader									= {};
	Graphics::Shader			m_FragmentShader								= {};
	Graphics::PipelineState		m_PSO											= {};
	Graphics::InputLayout		m_PSOInputLayout								= {};

	VkDescriptorSet				m_Set[Graphics::FRAMES_IN_FLIGHT]				= {};
};

void Instancing::StartUp() {
	m_Model									= ModelLoader::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/stormtrooper/scene.gltf");
	m_Model->Transformations.scaleHandler	= 0.009f;
	m_Model->Transformations.translation	= glm::vec3(0.0f, 0.0f, 0.0f);
	m_Model->Transformations.rotation		= glm::vec3(-100.0f, 0.0f, 0.0f);

	m_InstanceModelData.resize(MAX_MODELS);

	int num = 20;
	float offset_x = static_cast<float>(-num);
	float offset_y = static_cast<float>(-num);
	float offset_z = static_cast<float>(-num);
	float offset = 5.0f;

	uint32_t column_instance_index	= 1;
	uint32_t row_instance_index		= 1;
	uint32_t height_instance_index	= 1;

	for (int i = 0; i < MAX_MODELS; i++) {

		glm::vec3 translation = glm::vec3(0.0f);
		translation.x = offset_x;
		translation.y = offset_y;
		translation.z = offset_z;

		m_InstanceModelData[i].model = glm::translate(glm::mat4(1.0f), translation) * m_Model->GetModelMatrix();

		if (column_instance_index % num == 0 && row_instance_index % num == 0) {
			offset_x = static_cast<float>(-num);
			offset_z = static_cast<float>(-num);

			column_instance_index = 1;
			row_instance_index = 1;

			height_instance_index++;
			offset_y += offset;
		}
		else if (column_instance_index % num == 0) {
			offset_x = static_cast<float>(-num);

			column_instance_index = 1;

			row_instance_index++;
			offset_z += offset;
		}
		else {
			column_instance_index++;
			offset_x += offset;
		}
	}

	m_Camera.Init(glm::vec3(-0.82f, 15.11f, 37.4f), 45.0f, 267.4f, -31.8f, m_Width, m_Height);
	
	ResourceManager* rm = ResourceManager::Get();

	m_PSOInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstant) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },								// Scene GPU Data
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },															// Model GPU Data
			{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},															// Material Data
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(rm->GetTextures().size()), VK_SHADER_STAGE_FRAGMENT_BIT },	// Textures Array
			{ 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }																// Instance Transformation Matrices 
		}
	};
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

#ifdef RUNTIME_SHADER_COMPILATION
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		"../src/Samples/Instancing/vertex.glsl");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	"../src/Samples/Instancing/fragment.glsl");
#else
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		"vertex.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	"fragment.spv");
#endif


	m_ModelDataBuffer	= gfxDevice->CreateBuffer(sizeof(ModelGPUData));
	m_StorageBuffer		= gfxDevice->CreateStorageBuffer(MAX_MODELS * sizeof(InstanceModelData));

	gfxDevice->UpdateBuffer(m_StorageBuffer, 0, m_InstanceModelData.data(), sizeof(InstanceModelData) * MAX_MODELS);

	Graphics::PipelineStateDescription desc = {};
	desc.Name								= "Color Pipeline";
	desc.vertexShader						= &m_VertexShader;
	desc.fragmentShader						= &m_FragmentShader;
	desc.cullMode							= VK_CULL_MODE_BACK_BIT;
	desc.psoInputLayout						.push_back(m_PSOInputLayout);

	gfxDevice->CreatePipelineState(desc, m_PSO, *gfxDevice->GetSwapChain().RenderTarget.get());

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		m_SceneDataBuffer[i] = gfxDevice->CreateBuffer(sizeof(SceneGPUData));

		gfxDevice->CreateDescriptorSet(m_PSO.descriptorSetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[0], m_Set[i], m_SceneDataBuffer[i]);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[1], m_Set[i], m_ModelDataBuffer);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[2], m_Set[i], rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[3], m_Set[i], rm->GetTextures());
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[4], m_Set[i], m_StorageBuffer);
	}
}

void Instancing::CleanUp() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyBuffer(m_StorageBuffer);
	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_FragmentShader);
	gfxDevice->DestroyPipeline(m_PSO);

	m_Model.reset();
}

void Instancing::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	m_Camera.OnUpdate(deltaT, input);
	m_Model->OnUpdate(deltaT);

	m_SceneGPUData.time					= constantT;
	m_SceneGPUData.view					= m_Camera.ViewMatrix;
	m_SceneGPUData.proj					= m_Camera.ProjectionMatrix;

	m_ModelGPUData.flip_uv_vertically	= m_Model->FlipUvVertically;
	m_ModelGPUData.model				= m_Model->GetModelMatrix();
	m_ModelGPUData.normal				= glm::mat4(glm::mat3(glm::transpose(glm::inverse(m_SceneGPUData.view * m_ModelGPUData.model))));

	gfxDevice->UpdateBuffer(m_SceneDataBuffer[gfxDevice->GetCurrentFrameIndex()], &m_SceneGPUData);
	gfxDevice->UpdateBuffer(m_ModelDataBuffer, &m_ModelGPUData);
}

void Instancing::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->GetSwapChain().RenderTarget->Begin(commandBuffer);
	gfxDevice->BindDescriptorSet(m_Set[currentFrame], commandBuffer, m_PSO.pipelineLayout, 0, 1);
	
	VkDeviceSize offsets[] = { sizeof(uint32_t) * m_Model->TotalIndices };

	vkCmdBindVertexBuffers	(commandBuffer, 0, 1, &m_Model->DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer	(commandBuffer, m_Model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline		(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);

	for (const auto& mesh : m_Model->Meshes) {
		m_PushConstant.materialIndex = mesh.MaterialIndex;

		vkCmdPushConstants	(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstant), &m_PushConstant);
		vkCmdDrawIndexed	(commandBuffer, static_cast<uint32_t>(mesh.Indices.size()), MAX_MODELS, static_cast<uint32_t>(mesh.IndexOffset), static_cast<int32_t>(mesh.VertexOffset), 0);

		if (m_FirstPass) {
			m_DrawCalls++;
			m_ModelVertices += mesh.Vertices.size();
		}
	}

	if (m_FirstPass) {
		m_TotalVertices = m_ModelVertices * MAX_MODELS;
		m_FirstPass = false;
	}
}

void Instancing::RenderUI() {
	m_Camera.OnUIRender("Main Camera Settings");
	m_Model->OnUIRender();

	ImGui::Text("Total Draw Calls: %d", m_DrawCalls);
	ImGui::Text("Model Vertices: %d", m_ModelVertices);
	ImGui::Text("Total Vertices: %d", m_TotalVertices);

	ImGui::Separator();
	ImGui::ColorPicker4("Light Color", (float*)&m_SceneGPUData.lightColor);
	ImGui::DragFloat4("Light Postition", (float*)&m_SceneGPUData.lightPos, 0.002, -10.0f, 10.0f);

	ImGui::DragFloat("Light Intensity", &m_SceneGPUData.lightIntensity, 0.02f, 0.0f, 1.0f);
}

void Instancing::Resize(uint32_t width, uint32_t height) {
	m_Width		= width;
	m_Height	= height;
}

//RUN_APPLICATION(Instancing);
