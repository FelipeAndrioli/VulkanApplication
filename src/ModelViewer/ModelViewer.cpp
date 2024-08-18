#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "../Application.h"
#include "../Settings.h"

#include "../Graphics.h"
#include "../GraphicsDevice.h"
#include "../Renderer.h"

#include "../Assets/Camera.h"
#include "../Assets/Model.h"
#include "../Assets/Material.h"
#include "../Assets/Mesh.h"
#include "../Assets/Utils/MeshGenerator.h"
#include "../Utils/ModelLoader.h"
#include "../Utils/TextureLoader.h"

class ModelViewer : public Engine::Application::IScene {
public:
	ModelViewer() {
		settings.Title = "ModelViewer.exe";
		settings.Width = 1600;
		settings.Height = 900;
		settings.uiEnabled = true;
		settings.renderSkybox = true;
	};

	virtual void StartUp() override;
	virtual void CleanUp() override;
	virtual void Update(float d, Engine::InputSystem::Input& input) override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) override;
	virtual void RenderUI() override;
	virtual bool IsDone(Engine::InputSystem::Input& input) override;
	virtual void Resize(uint32_t width, uint32_t height) override;

	void Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, const PipelineState& pso);
private:
	Assets::Camera* m_Camera = nullptr;

	std::shared_ptr<Model> m_Model;

	Engine::Application::SceneGPUData m_SceneGPUData = {};

	std::vector<Assets::Material> m_Materials;
	std::vector<Texture> m_Textures;

	Buffer modelBuffer[Engine::Graphics::FRAMES_IN_FLIGHT] = {};
	Buffer materialsBuffer[Engine::Graphics::FRAMES_IN_FLIGHT] = {};
	Buffer sceneBuffer[Engine::Graphics::FRAMES_IN_FLIGHT] = {};

	VkDescriptorSet ModelDescriptorSets[FRAMES_IN_FLIGHT] = {};
	VkDescriptorSet GlobalDescriptorSets[FRAMES_IN_FLIGHT] = {};

	PipelineState m_ColorPipeline = {};
	PipelineState m_WireframePipeline = {};

	Shader defaultVertexShader = {};
	Shader colorFragShader = {};
	Shader wireframeFragShader = {};
	Shader skyboxVertexShader = {};
	Shader skyboxFragShader = {};

	uint32_t m_ScreenWidth = 0;
	uint32_t m_ScreenHeight = 0;
};

void ModelViewer::StartUp() {

	m_ScreenWidth = settings.Width;
	m_ScreenHeight = settings.Height;

	m_Camera = new Assets::Camera(glm::vec3(0.6f, 2.1f, 9.2f), 45.0f, -113.0f, -1.7f, m_ScreenWidth, m_ScreenHeight);

	Renderer::Init();

	m_Model = Renderer::LoadModel(
		"C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf",
		m_Materials, m_Textures);
	m_Model->Name = "Dragon";
	m_Model->Transformations.scaleHandler = 20.0f;
	m_Model->Transformations.translation.x = 0.0f;
	m_Model->Transformations.translation.y = 0.0f;

	/*
	m_Model = Renderer::LoadModel(
		"C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj",
		m_Materials, m_Textures);

	m_Model->Name = "Sponza";
	m_Model->Transformations.scaleHandler = 0.008f;
	m_Model->Transformations.translation.x = -10.8f;
	m_Model->Transformations.translation.y = -2.5f;
	m_Model->Transformations.rotation.y = 45.0f;
	m_Model->FlipTexturesVertically = true;
	*/

	// Buffers initialization
	// GPU Data Buffer Begin

	std::vector<Assets::MeshMaterialData> meshMaterialData;

	for (const auto& material : m_Materials) {
		meshMaterialData.push_back(material.MaterialData);
	}

	GraphicsDevice* gfxDevice = GetDevice();

	for (int i = 0; i < Engine::Graphics::FRAMES_IN_FLIGHT; i++) {
		modelBuffer[i] = gfxDevice->CreateBuffer(sizeof(Engine::Application::ObjectGPUData));

		materialsBuffer[i] = gfxDevice->CreateBuffer(sizeof(Assets::MeshMaterialData) * m_Materials.size());
		gfxDevice->WriteBuffer(materialsBuffer[i], meshMaterialData.data());

		sceneBuffer[i] = gfxDevice->CreateBuffer(sizeof(Engine::Application::SceneGPUData));
	}
	// GPU Data Buffer End

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, defaultVertexShader, "./Shaders/default_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, colorFragShader, "./Shaders/color_ps.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, wireframeFragShader, "./Shaders/wireframe_frag.spv");

	PipelineStateDescription psoDesc = {};
	psoDesc.Name = "Texture Pipeline";
	psoDesc.vertexShader = &defaultVertexShader;
	psoDesc.fragmentShader = &colorFragShader;
	psoDesc.cullMode = VK_CULL_MODE_BACK_BIT;
	psoDesc.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	psoDesc.polygonMode = VK_POLYGON_MODE_FILL;
	psoDesc.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	psoDesc.pipelineExtent = { m_ScreenWidth, m_ScreenHeight };

	InputLayout modelInputLayout = {
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }
		}
	};

	psoDesc.psoInputLayout.push_back(modelInputLayout);

	InputLayout sceneInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_Textures.size()), VK_SHADER_STAGE_FRAGMENT_BIT },
		}
	};

	psoDesc.psoInputLayout.push_back(sceneInputLayout);

	gfxDevice->CreatePipelineState(psoDesc, m_ColorPipeline);

	psoDesc.Name = "Wireframe Pipeline";
	psoDesc.fragmentShader = &wireframeFragShader;
	psoDesc.polygonMode = VK_POLYGON_MODE_LINE;
	psoDesc.lineWidth = 3.0f;
	
	gfxDevice->CreatePipelineState(psoDesc, m_WireframePipeline);

	// Renderable Objects Descriptor Sets Begin
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_ColorPipeline.descriptorSetLayout[0], ModelDescriptorSets[i]);
		gfxDevice->WriteDescriptor(
			modelInputLayout.bindings[0],
			ModelDescriptorSets[i], 
			*modelBuffer[i].Handle,
			modelBuffer[i].Size,
			modelBuffer[i].Offset
		);
	}
	// Renderable Objects Descriptor Sets End 

	// Global Descriptor Sets Begina
	for (int i = 0; i < Engine::Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_ColorPipeline.descriptorSetLayout[1], GlobalDescriptorSets[i]);

		gfxDevice->WriteDescriptor(
			sceneInputLayout.bindings[0],
			GlobalDescriptorSets[i],
			*sceneBuffer[i].Handle,
			sceneBuffer[i].Size,
			sceneBuffer[i].Offset
		);
		gfxDevice->WriteDescriptor(
			sceneInputLayout.bindings[1],
			GlobalDescriptorSets[i],
			*materialsBuffer[i].Handle,
			materialsBuffer[i].Size,
			materialsBuffer[i].Offset
		);

		gfxDevice->WriteDescriptor(sceneInputLayout.bindings[2], GlobalDescriptorSets[i], m_Textures);
	}
	// Global Descriptor Sets End
}

void ModelViewer::CleanUp() {
	GraphicsDevice* gfxDevice = GetDevice();

	gfxDevice->DestroyShader(defaultVertexShader);
	gfxDevice->DestroyShader(colorFragShader);
	gfxDevice->DestroyShader(wireframeFragShader);

	gfxDevice->DestroyPipeline(m_ColorPipeline);
	gfxDevice->DestroyPipeline(m_WireframePipeline);
	Renderer::Destroy();

	for (auto texture : m_Textures) {
		gfxDevice->DestroyImage(texture);
	}

	m_Textures.clear();
	m_Materials.clear();

	delete m_Camera;
}

void ModelViewer::Update(float d, Engine::InputSystem::Input& input) {
	m_Camera->OnUpdate(d, input);
	m_Model->OnUpdate(d);
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	GraphicsDevice* gfxDevice = GetDevice();

	VkDeviceSize offsets[] = { sizeof(uint32_t) * m_Model->TotalIndices };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Model->DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_Model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

	m_SceneGPUData.view = m_Camera->ViewMatrix;
	m_SceneGPUData.proj = m_Camera->ProjectionMatrix;

	gfxDevice->UpdateBuffer(sceneBuffer[currentFrame], &m_SceneGPUData);
	gfxDevice->BindDescriptorSet(GlobalDescriptorSets[currentFrame], commandBuffer, m_ColorPipeline.pipelineLayout, 1, 1);

	Render(currentFrame, commandBuffer, m_ColorPipeline);

	if (settings.wireframeEnabled)
		Render(currentFrame, commandBuffer, m_WireframePipeline);

	if (settings.renderSkybox)
		Renderer::RenderSkybox(commandBuffer, *m_Camera);
}

void ModelViewer::Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, const PipelineState& pso) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso.pipeline);

	GraphicsDevice* gfxDevice = GetDevice();

	gfxDevice->BindDescriptorSet(ModelDescriptorSets[currentFrame], commandBuffer, pso.pipelineLayout, 0, 1);

	Engine::Application::ObjectGPUData objectGPUData = Engine::Application::ObjectGPUData();
	objectGPUData.model = m_Model->GetModelMatrix();

	gfxDevice->UpdateBuffer(modelBuffer[currentFrame], &objectGPUData);

	for (const auto& mesh : m_Model->Meshes) {
		vkCmdPushConstants(
			commandBuffer,
			pso.pipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(int),
			&mesh.MaterialIndex
		);

		vkCmdDrawIndexed(
			commandBuffer,
			static_cast<uint32_t>(mesh.Indices.size()),
			1,
			static_cast<uint32_t>(mesh.IndexOffset),
			static_cast<int32_t>(mesh.VertexOffset),
			0
		);
	}
}

void ModelViewer::RenderUI() {
	ImGui::SeparatorText("Model Viewer");
	ImGui::Checkbox("Enable Wireframe", &settings.wireframeEnabled);
	ImGui::Checkbox("Render Skybox", &settings.renderSkybox);

	m_Camera->OnUIRender();
	m_Model->OnUIRender();
}

bool ModelViewer::IsDone(Engine::InputSystem::Input& input) {
	return input.Keys[GLFW_KEY_ESCAPE].IsPressed;
}

void ModelViewer::Resize(uint32_t width, uint32_t height) {
	m_Camera->Resize(width, height);
}

RUN_APPLICATION(ModelViewer)
