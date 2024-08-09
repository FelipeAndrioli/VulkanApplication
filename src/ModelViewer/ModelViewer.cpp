#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "../ApplicationCore.h"
#include "../Settings.h"

#include "../Graphics.h"
#include "../GraphicsDevice.h"

#include "../Assets/Camera.h"
#include "../Assets/Scene.h"
#include "../Assets/Object.h"
#include "../Assets/Shader.h"
#include "../Assets/Pipeline.h"
#include "../Assets/Material.h"
#include "../Assets/Utils/MeshGenerator.h"
#include "../Utils/ModelLoader.h"
#include "../Utils/TextureLoader.h"

class ModelViewer : public Engine::ApplicationCore::IScene {
public:
	ModelViewer() {};
	ModelViewer(Engine::Settings& settings) : m_Settings(settings) {};

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

	Engine::Settings m_Settings = {};

	// TODO: make it an array
	Assets::Object m_Model = {};
		
	Engine::ApplicationCore::SceneGPUData m_SceneGPUData;

	std::vector<Assets::Material> m_Materials;
	std::vector<Texture> m_Textures;
	Texture m_Skybox;

	GPUBuffer m_GPUDataBuffer[Engine::Graphics::FRAMES_IN_FLIGHT];
	GPUBuffer m_SceneGeometryBuffer;
		
	VkDescriptorSet ModelDescriptorSets[FRAMES_IN_FLIGHT];
	VkDescriptorSet GlobalDescriptorSets[FRAMES_IN_FLIGHT];

	PipelineState m_TexturedPipeline;
	PipelineState m_WireframePipeline;
	PipelineState m_ColoredPipeline;
	PipelineState m_SkyboxPipeline;

	Shader defaultVertexShader = {};
	Shader texturedFragShader = {};
	Shader wireframeFragShader = {};
	Shader coloredFragShader = {};
	Shader skyboxVertexShader = {};
	Shader skyboxFragShader = {};

	static const int OBJECT_BUFFER_INDEX = 0;
	static const int MATERIAL_BUFFER_INDEX = 1;
	static const int SCENE_BUFFER_INDEX = 2;
	static const int INDEX_BUFFER_INDEX = 0;			// :) 
	static const int VERTEX_BUFFER_INDEX = 1;
	const std::string DEFAULT_GRAPHICS_PIPELINE = "defaultPipeline";

	uint32_t m_ScreenWidth = 0;
	uint32_t m_ScreenHeight = 0;
};

void ModelViewer::StartUp() {

	m_ScreenWidth = m_Settings.Width;
	m_ScreenHeight = m_Settings.Height;

	m_Camera = new Assets::Camera(glm::vec3(0.6f, 2.1f, 9.2f), 45.0f, -113.0f, -1.7f, m_ScreenWidth, m_ScreenHeight);

	/*
	m_Model.ID = "Ship";
	m_Model.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/ship_pinnace_4k.gltf/ship_pinnace_4k.gltf";
	m_Model.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/ship_pinnace_4k.gltf/";
	m_Model.Transformations.scaleHandler = 0.2f;
	m_Model.Transformations.translation.x = -10.8f;
	m_Model.Transformations.translation.y = -2.5f;
	m_Model.Transformations.rotation.y = 45.0f;

	m_Model.ID = "Sponza";
	m_Model.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj";
	m_Model.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/";
	m_Model.Transformations.scaleHandler = 0.008f;
	
	m_Model.Transformations.translation.x = -10.8f;
	m_Model.Transformations.translation.y = -2.5f;
	m_Model.Transformations.rotation.y = 45.0f;
	*/

	m_Model.ID = "Dragon";
	m_Model.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf";
	m_Model.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/";
	m_Model.Transformations.scaleHandler = 20.0f;
	m_Model.Transformations.translation.x = 0.0f;
	m_Model.Transformations.translation.y = 0.0f;

	m_Model.FlipTexturesVertically = true;

	ModelLoader::LoadModelAndMaterials(m_Model, m_Materials, m_Textures);

	std::vector<std::string> cubeTextures = {
		"./Textures/right.jpg",
		"./Textures/left.jpg",
		"./Textures/top.jpg",
		"./Textures/bottom.jpg",
		"./Textures/front.jpg",
		"./Textures/back.jpg",
	};

	//m_Skybox = std::make_unique<struct Assets::Texture>(Utils::TextureLoader::LoadCubemapTexture("./Textures/immenstadter_horn_2k.hdr", *m_VulkanEngine.get()));
	m_Skybox = TextureLoader::LoadCubemapTexture(cubeTextures);

	// Buffers initialization
	// GPU Data Buffer Begin
	VkDeviceSize objectBufferSize = sizeof(Engine::ApplicationCore::ObjectGPUData) * 1; // TODO: make it an array
	VkDeviceSize materialsBufferSize = sizeof(Assets::MeshMaterialData) * m_Materials.size();
	VkDeviceSize sceneBufferSize = sizeof(Engine::ApplicationCore::SceneGPUData);
	VkDeviceSize gpuBufferSize = objectBufferSize + materialsBufferSize + sceneBufferSize;

	std::vector<Assets::MeshMaterialData> meshMaterialData;

	for (const auto& material : m_Materials) {
		meshMaterialData.push_back(material.MaterialData);
	}

	BufferDescription gpuDataBufferDesc = {};
	gpuDataBufferDesc.BufferSize = gpuBufferSize;
	gpuDataBufferDesc.MemoryProperty = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	gpuDataBufferDesc.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	gpuDataBufferDesc.Chunks.push_back({ sizeof(Engine::ApplicationCore::ObjectGPUData), objectBufferSize });
	gpuDataBufferDesc.Chunks.push_back({ sizeof(Assets::MeshMaterialData), materialsBufferSize });
	gpuDataBufferDesc.Chunks.push_back({ sizeof(Engine::ApplicationCore::SceneGPUData), sceneBufferSize });

	GraphicsDevice* gfxDevice = GetDevice();

	for (int i = 0; i < Engine::Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateBuffer(gpuDataBufferDesc, m_GPUDataBuffer[i], gpuBufferSize);
		gfxDevice->WriteBuffer(m_GPUDataBuffer[i], meshMaterialData.data(), m_GPUDataBuffer[i].Description.Chunks[MATERIAL_BUFFER_INDEX].ChunkSize, m_GPUDataBuffer[i].Description.Chunks[OBJECT_BUFFER_INDEX].ChunkSize);
	}
	// GPU Data Buffer End

	// Scene Geometry Buffer Begin
	VkDeviceSize sceneGeometryBufferSize = sizeof(uint32_t) * m_Model.IndicesAmount + sizeof(Assets::Vertex) * m_Model.VerticesAmount;
	/*
	VkDeviceSize bufferSize = sizeof(uint32_t) * p_ActiveScene->Indices.size()
		+ sizeof(Assets::Vertex) * p_ActiveScene->Vertices.size();
	*/

	BufferDescription sceneGeometryBufferDesc = {};
	sceneGeometryBufferDesc.BufferSize = sceneGeometryBufferSize;
	sceneGeometryBufferDesc.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	sceneGeometryBufferDesc.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	sceneGeometryBufferDesc.Chunks.push_back({ sizeof(uint32_t), sizeof(uint32_t) * m_Model.IndicesAmount });
	sceneGeometryBufferDesc.Chunks.push_back({ sizeof(Assets::Vertex), sizeof(Assets::Vertex) * m_Model.VerticesAmount });

	gfxDevice->CreateBuffer(sceneGeometryBufferDesc, m_SceneGeometryBuffer, sceneGeometryBufferSize);
	
	size_t indexOffset = 0;

	for (auto mesh : m_Model.Meshes) {
		gfxDevice->WriteBuffer(m_SceneGeometryBuffer, mesh.Indices.data(), sizeof(uint32_t) * mesh.Indices.size(), indexOffset);
		indexOffset += sizeof(uint32_t) * mesh.Indices.size();
	}

	size_t vertexOffset = indexOffset;

	for (auto mesh : m_Model.Meshes) {
		gfxDevice->WriteBuffer(m_SceneGeometryBuffer, mesh.Vertices.data(), sizeof(Assets::Vertex) * mesh.Vertices.size(), vertexOffset);
		vertexOffset += sizeof(Assets::Vertex) * mesh.Vertices.size();
	}
	// Scene Geometry Buffer End

	// TODO: remove descriptor pool creation from here
	gfxDevice->CreateDescriptorPool();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, defaultVertexShader, "./Shaders/default_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, texturedFragShader, "./Shaders/textured_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, wireframeFragShader, "./Shaders/wireframe_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, coloredFragShader, "./Shaders/colored_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, skyboxVertexShader, "./Shaders/skybox_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, skyboxFragShader, "./Shaders/skybox_frag.spv");

	PipelineStateDescription psoDesc = {};
	psoDesc.Name = "Texture Pipeline";
	psoDesc.vertexShader = &defaultVertexShader;
	psoDesc.fragmentShader = &texturedFragShader;
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
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		}
	};

	psoDesc.psoInputLayout.push_back(sceneInputLayout);

	gfxDevice->CreatePipelineState(psoDesc, m_TexturedPipeline);


	psoDesc.Name = "Wireframe Pipeline";
	psoDesc.fragmentShader = &wireframeFragShader;
	psoDesc.polygonMode = VK_POLYGON_MODE_LINE;
	psoDesc.lineWidth = 3.0f;
	
	gfxDevice->CreatePipelineState(psoDesc, m_WireframePipeline);

	//psoDesc.psoInputLayout.clear();
	//psoDesc.psoInputLayout.push_back(sceneInputLayout);
	psoDesc.Name = "Skybox Pipeline";
	psoDesc.lineWidth = 1.0f;
	psoDesc.vertexShader = &skyboxVertexShader;
	psoDesc.fragmentShader = &skyboxFragShader;
	psoDesc.polygonMode = VK_POLYGON_MODE_FILL;

	gfxDevice->CreatePipelineState(psoDesc, m_SkyboxPipeline);

	// Renderable Objects Descriptor Sets Begin
	VkDeviceSize objectBufferOffset = 0 * m_GPUDataBuffer[0].Description.Chunks[OBJECT_BUFFER_INDEX].DataSize;

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_TexturedPipeline.descriptorSetLayout[0], ModelDescriptorSets[i]);
		gfxDevice->WriteDescriptor(
			modelInputLayout.bindings[0],
			ModelDescriptorSets[i], 
			m_GPUDataBuffer[i].Handle, 
			m_GPUDataBuffer[i].Description.Chunks[OBJECT_BUFFER_INDEX].ChunkSize, 
			objectBufferOffset
		);
	}
	// Renderable Objects Descriptor Sets End 

	// Global Descriptor Sets Begin
	for (int i = 0; i < Engine::MAX_FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_TexturedPipeline.descriptorSetLayout[1], GlobalDescriptorSets[i]);
		gfxDevice->WriteDescriptor(
			sceneInputLayout.bindings[0],
			GlobalDescriptorSets[i],
			m_GPUDataBuffer[i].Handle,
			m_GPUDataBuffer[i].Description.Chunks[SCENE_BUFFER_INDEX].ChunkSize,
			m_GPUDataBuffer[i].Description.Chunks[OBJECT_BUFFER_INDEX].ChunkSize + m_GPUDataBuffer[i].Description.Chunks[MATERIAL_BUFFER_INDEX].ChunkSize
		);

		gfxDevice->WriteDescriptor(
			sceneInputLayout.bindings[1],
			GlobalDescriptorSets[i],
			m_GPUDataBuffer[i].Handle,
			m_GPUDataBuffer[i].Description.Chunks[MATERIAL_BUFFER_INDEX].ChunkSize,
			m_GPUDataBuffer[i].Description.Chunks[OBJECT_BUFFER_INDEX].ChunkSize
		);

		gfxDevice->WriteDescriptor(sceneInputLayout.bindings[2], GlobalDescriptorSets[i], m_Textures);
		gfxDevice->WriteDescriptor(sceneInputLayout.bindings[3], GlobalDescriptorSets[i], m_Skybox);
	}
	// Global Descriptor Sets End
}

void ModelViewer::CleanUp() {
	GraphicsDevice* gfxDevice = GetDevice();

	m_Model.ResetResources();

	gfxDevice->DestroyShader(defaultVertexShader);
	gfxDevice->DestroyShader(texturedFragShader);
	gfxDevice->DestroyShader(wireframeFragShader);
	gfxDevice->DestroyShader(coloredFragShader);
	gfxDevice->DestroyShader(skyboxVertexShader);
	gfxDevice->DestroyShader(skyboxFragShader);

	gfxDevice->DestroyPipeline(m_TexturedPipeline);
	gfxDevice->DestroyPipeline(m_WireframePipeline);
	gfxDevice->DestroyPipeline(m_ColoredPipeline);
	gfxDevice->DestroyPipeline(m_SkyboxPipeline);

	gfxDevice->DestroyBuffer(m_SceneGeometryBuffer);

	for (int i = 0; i < Engine::Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->DestroyBuffer(m_GPUDataBuffer[i]);
	}

	gfxDevice->DestroyTexture(m_Skybox);

	for (auto texture : m_Textures) {
		gfxDevice->DestroyTexture(texture);
	}

	m_Textures.clear();
	m_Materials.clear();

	delete m_Camera;
}

void ModelViewer::Update(float d, Engine::InputSystem::Input& input) {
	m_Camera->OnUpdate(d, input);
	m_Model.OnUpdate(d);
}

void RenderSkybox(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	GraphicsDevice* gfxDevice = GetDevice();

	//VkDeviceSize offsets[] = { p_ActiveScene->VertexOffset };
	VkDeviceSize offsets[] = { sizeof(uint32_t) * m_Model.IndicesAmount };

	vkCmdBindVertexBuffers(
		commandBuffer,
		0,
		1,
		&m_SceneGeometryBuffer.Handle,
		offsets
	);

	vkCmdBindIndexBuffer(
		commandBuffer,
		m_SceneGeometryBuffer.Handle,
		0,
		VK_INDEX_TYPE_UINT32
	);

	m_SceneGPUData.view = m_Camera->ViewMatrix;
	m_SceneGPUData.proj = m_Camera->ProjectionMatrix;

	VkDeviceSize sceneBufferOffset = m_GPUDataBuffer[currentFrame].Description.Chunks[OBJECT_BUFFER_INDEX].ChunkSize 
		+ m_GPUDataBuffer[currentFrame].Description.Chunks[MATERIAL_BUFFER_INDEX].ChunkSize;

	gfxDevice->UpdateBuffer(m_GPUDataBuffer[currentFrame], sceneBufferOffset, &m_SceneGPUData, sizeof(Engine::ApplicationCore::SceneGPUData));
	//m_GPUDataBuffer[currentFrame]->Update(sceneBufferOffset, &m_SceneGPUData, sizeof(Engine::ApplicationCore::SceneGPUData));

	gfxDevice->BindDescriptorSet(GlobalDescriptorSets[currentFrame], commandBuffer, m_TexturedPipeline.pipelineLayout, 1, 1);
	//m_GlobalDescriptorSets[currentFrame]->Bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipelineLayout->GetHandle());

	
	Render(currentFrame, commandBuffer, m_TexturedPipeline);
	//Render(currentFrame, commandBuffer, m_ColoredPipeline.pipeline);

	if (m_Settings.wireframeEnabled)
		Render(currentFrame, commandBuffer, m_WireframePipeline);

	if (m_Settings.renderSkybox)
		RenderSkybox(commandBuffer, m_SkyboxPipeline.pipeline);
}

void ModelViewer::Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, const PipelineState& pso) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso.pipeline);

	GraphicsDevice* gfxDevice = GetDevice();

	gfxDevice->BindDescriptorSet(ModelDescriptorSets[currentFrame], commandBuffer, pso.pipelineLayout, 0, 1);
	//m_Model.DescriptorSets[currentFrame]->Bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipelineLayout->GetHandle());

	Engine::ApplicationCore::ObjectGPUData objectGPUData = Engine::ApplicationCore::ObjectGPUData();
	objectGPUData.model = m_Model.GetModelMatrix();

	//VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
	//VkDeviceSize objectBufferOffset = 0 * m_GPUDataBuffer[currentFrame]->Chunks[OBJECT_BUFFER_INDEX].DataSize;
	VkDeviceSize objectBufferOffset = 0 * m_GPUDataBuffer[currentFrame].Description.Chunks[OBJECT_BUFFER_INDEX].DataSize;
	
	gfxDevice->UpdateBuffer(m_GPUDataBuffer[currentFrame], objectBufferOffset, &objectGPUData, sizeof(Engine::ApplicationCore::ObjectGPUData));
	//m_GPUDataBuffer[currentFrame]->Update(objectBufferOffset, &objectGPUData, sizeof(Engine::ApplicationCore::ObjectGPUData));

	for (const auto& mesh : m_Model.Meshes) {
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
	ImGui::Checkbox("Enable Wireframe", &m_Settings.wireframeEnabled);
	ImGui::Checkbox("Render SKybox", &m_Settings.renderSkybox);

	m_Camera->OnUIRender();
	m_Model.OnUIRender();
}

bool ModelViewer::IsDone(Engine::InputSystem::Input& input) {
	return input.Keys[GLFW_KEY_ESCAPE].IsPressed;
}

void ModelViewer::Resize(uint32_t width, uint32_t height) {
	m_Camera->Resize(width, height);
}

int main() {

	Engine::Settings settings = {};
	settings.Title = "ModelViewer.exe";
	settings.Width = 1600;
	settings.Height = 900;
	settings.uiEnabled = true;
	settings.renderSkybox = true;

	std::unique_ptr<Engine::ApplicationCore> app = std::make_unique<Engine::ApplicationCore>(settings);
	ModelViewer modelViewer = ModelViewer(settings);

	app->RunApplication(modelViewer);

	app.reset();

	return 0;
}