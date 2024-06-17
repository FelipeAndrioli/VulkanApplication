#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "../ApplicationCore.h"
#include "../Input/Input.h"
#include "../BufferHelper.h"
#include "../Buffer.h"
#include "../DescriptorPool.h"
#include "../DescriptorSetLayout.h"
#include "../DescriptorSets.h"
#include "../Pipeline.h"
#include "../PipelineLayout.h"
#include "../RenderPass.h"

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

	virtual void StartUp(Engine::VulkanEngine& vulkanEngine) override;
	virtual void CleanUp() override;
	virtual void Update(float d, Engine::InputSystem::Input& input) override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) override;
	virtual void RenderUI() override;
	virtual bool IsDone(Engine::InputSystem::Input& input) override;

	void Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline);
private:
	Assets::Camera* m_Camera;
	std::unique_ptr<struct Assets::Texture> m_Skybox;

	Engine::Settings m_Settings = {};

	// TODO: make it an array
	Assets::Object m_Ship = {};
		
	Engine::ApplicationCore::SceneGPUData m_SceneGPUData;

	std::vector<Assets::Material> m_Materials;
	std::vector<Assets::Texture> m_Textures;

	std::unique_ptr<Engine::Buffer> m_GPUDataBuffer;
	std::unique_ptr<Engine::Buffer> m_SceneGeometryBuffer;

	std::unique_ptr<Engine::DescriptorPool> m_DescriptorPool;

	std::unique_ptr<Engine::DescriptorSetLayout> m_ObjectGPUDataDescriptorSetLayout;
	std::unique_ptr<Engine::DescriptorSetLayout> m_GlobalDescriptorSetLayout;

	std::unique_ptr<Engine::DescriptorSets> m_GlobalDescriptorSets;
	
	std::unique_ptr<Engine::PipelineLayout> m_MainPipelineLayout;

	std::unique_ptr<Engine::GraphicsPipeline> m_TexturedPipeline;
	std::unique_ptr<Engine::GraphicsPipeline> m_WireframePipeline;
	std::unique_ptr<Engine::GraphicsPipeline> m_ColoredPipeline;
	std::unique_ptr<Engine::GraphicsPipeline> m_SkyboxPipeline;

	static const int OBJECT_BUFFER_INDEX = 0;
	static const int MATERIAL_BUFFER_INDEX = 1;
	static const int SCENE_BUFFER_INDEX = 2;
	static const int INDEX_BUFFER_INDEX = 0;			// :) 
	static const int VERTEX_BUFFER_INDEX = 1;
	const std::string DEFAULT_GRAPHICS_PIPELINE = "defaultPipeline";
};

void ModelViewer::StartUp(Engine::VulkanEngine& vulkanEngine) {

	m_Camera = new Assets::Camera(glm::vec3(0.0f, 0.0f, 3.0f), 45.0f, 1900, 600);

	m_Ship.ID = "Ship";
	m_Ship.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/ship_pinnace_4k.gltf/ship_pinnace_4k.gltf";
	m_Ship.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/ship_pinnace_4k.gltf/";
	m_Ship.FlipTexturesVertically = false;
	m_Ship.Transformations.translation.x = -10.8f;
	m_Ship.Transformations.translation.y = -2.5f;
	m_Ship.Transformations.rotation.y = 45.0f;
	m_Ship.Transformations.scaleHandler = 0.2f;

	Engine::Utils::ModelLoader::LoadModelAndMaterials(m_Ship, m_Materials, m_Textures, vulkanEngine);

	std::vector<std::string> cubeTextures = {
		"./Textures/right.jpg",
		"./Textures/left.jpg",
		"./Textures/top.jpg",
		"./Textures/bottom.jpg",
		"./Textures/front.jpg",
		"./Textures/back.jpg",
	};

	m_Skybox = std::make_unique<Assets::Texture>(Engine::Utils::TextureLoader::LoadCubemapTexture(cubeTextures, vulkanEngine));

	// Buffers initialization
	// GPU Data Buffer Begin
	VkDeviceSize objectBufferSize = sizeof(Engine::ApplicationCore::ObjectGPUData) * 1; // TODO: make it an array
	VkDeviceSize materialsBufferSize = sizeof(Assets::MeshMaterialData) * m_Materials.size();
	VkDeviceSize sceneBufferSize = sizeof(Engine::ApplicationCore::SceneGPUData);

	m_GPUDataBuffer = std::make_unique<class Engine::Buffer>(
		Engine::MAX_FRAMES_IN_FLIGHT,
		vulkanEngine,
		objectBufferSize + materialsBufferSize + sceneBufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);
	m_GPUDataBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_GPUDataBuffer->NewChunk({ sizeof(Engine::ApplicationCore::ObjectGPUData), objectBufferSize });
	m_GPUDataBuffer->NewChunk({ sizeof(Assets::MeshMaterialData), materialsBufferSize });
	m_GPUDataBuffer->NewChunk({ sizeof(Engine::ApplicationCore::SceneGPUData), sceneBufferSize });

	std::vector<Assets::MeshMaterialData> meshMaterialData;

	for (const auto& material : m_Materials) {
		meshMaterialData.push_back(material.MaterialData);
	}

	Engine::BufferHelper::AppendData(
		vulkanEngine,
		meshMaterialData,
		*m_GPUDataBuffer.get(),
		0,
		m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize
	);
	// GPU Data Buffer End

	// Scene Geometry Buffer Begin
	VkDeviceSize bufferSize = sizeof(uint32_t) * m_Ship.IndicesAmount + sizeof(Assets::Vertex) * m_Ship.VerticesAmount;
	/*
	VkDeviceSize bufferSize = sizeof(uint32_t) * p_ActiveScene->Indices.size()
		+ sizeof(Assets::Vertex) * p_ActiveScene->Vertices.size();
	*/

	m_SceneGeometryBuffer = std::make_unique<class Engine::Buffer>(
		Engine::MAX_FRAMES_IN_FLIGHT,
		vulkanEngine,
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);

	m_SceneGeometryBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//m_SceneGeometryBuffer->NewChunk({ sizeof(uint32_t), sizeof(uint32_t) * p_ActiveScene->Indices.size() });
	m_SceneGeometryBuffer->NewChunk({ sizeof(uint32_t), sizeof(uint32_t) * m_Ship.IndicesAmount });
	//m_SceneGeometryBuffer->NewChunk({ sizeof(Assets::Vertex), sizeof(Assets::Vertex) * p_ActiveScene->Vertices.size() });
	m_SceneGeometryBuffer->NewChunk({ sizeof(Assets::Vertex), sizeof(Assets::Vertex) * m_Ship.VerticesAmount });

	for (auto mesh : m_Ship.Meshes) {
		Engine::BufferHelper::AppendData(
			vulkanEngine,
			//p_ActiveScene->Indices,
			mesh.Indices,
			*m_SceneGeometryBuffer.get(),
			0,
			0
		);

		Engine::BufferHelper::AppendData(
			vulkanEngine,
			mesh.Vertices,
			*m_SceneGeometryBuffer.get(),
			0,
			m_SceneGeometryBuffer->Chunks[INDEX_BUFFER_INDEX].ChunkSize
		);
	}

	/*
	Engine::BufferHelper::AppendData(
		vulkanEngine,
		//p_ActiveScene->Indices,
		p_ActiveScene->Indices,
		*m_SceneGeometryBuffer.get(),
		0,
		0
	);

	Engine::BufferHelper::AppendData(
		vulkanEngine,
		p_ActiveScene->Vertices,
		*m_SceneGeometryBuffer.get(),
		0,
		m_SceneGeometryBuffer->Chunks[INDEX_BUFFER_INDEX].ChunkSize
	);
	*/
	// Scene Geometry Buffer End

	Engine::DescriptorPoolBuilder descriptorPoolBuilder = {};
	m_DescriptorPool = descriptorPoolBuilder.AddDescriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.AddDescriptorCount(10)
		.AddBinding()
		.AddDescriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		.AddDescriptorCount(10)
		.AddBinding()
		.SetMaxSets(50)
		.Build(vulkanEngine.GetLogicalDevice().GetHandle());

	Engine::DescriptorSetLayoutBuild descriptorLayoutBuild = {};
	m_ObjectGPUDataDescriptorSetLayout = descriptorLayoutBuild.NewBinding(0)
		.SetDescriptorCount(1)
		.SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.SetStage(VK_SHADER_STAGE_VERTEX_BIT)
		.SetResource(*m_GPUDataBuffer.get())
		.SetBufferSize(m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize)
		.SetBufferOffset(0)
		.Add()
		.Build(vulkanEngine.GetLogicalDevice().GetHandle());

	m_GlobalDescriptorSetLayout = descriptorLayoutBuild.NewBinding(0)
		.SetDescriptorCount(1)
		.SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.SetStage(VK_SHADER_STAGE_VERTEX_BIT)
		.SetResource(*m_GPUDataBuffer.get())
		.SetBufferSize(m_GPUDataBuffer->Chunks[SCENE_BUFFER_INDEX].ChunkSize)
		.SetBufferOffset(m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize + m_GPUDataBuffer->Chunks[MATERIAL_BUFFER_INDEX].ChunkSize)
		.Add()
		.NewBinding(1)
		.SetDescriptorCount(1)
		.SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.SetStage(VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetResource(*m_GPUDataBuffer.get())
		.SetBufferSize(m_GPUDataBuffer->Chunks[MATERIAL_BUFFER_INDEX].ChunkSize)
		.SetBufferOffset(m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize)
		.Add()
		.NewBinding(2)
		.SetDescriptorCount(static_cast<uint32_t>(m_Textures.size()))
		.SetType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		.SetStage(VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetResource(m_Textures)
		.SetBufferSize(0)
		.SetBufferOffset(0)
		.Add()
		.NewBinding(3)
		.SetDescriptorCount(1)
		.SetType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		.SetStage(VK_SHADER_STAGE_FRAGMENT_BIT)
		.SetResource({ *m_Skybox.get() })
		.SetBufferSize(0)
		.SetBufferOffset(0)
		.Add()
		.Build(vulkanEngine.GetLogicalDevice().GetHandle());

	// Renderable Objects Descriptor Sets Begin
	VkDeviceSize objectBufferOffset = 0 * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
	m_ObjectGPUDataDescriptorSetLayout->UpdateOffset(0, objectBufferOffset);

	m_Ship.DescriptorSets = std::make_unique<class Engine::DescriptorSets>(
		vulkanEngine.GetLogicalDevice().GetHandle(),
		m_DescriptorPool->GetHandle(),
		*m_ObjectGPUDataDescriptorSetLayout.get(),
		0,
		1
	);
	// Renderable Objects Descriptor Sets End 

	// Global Descriptor Sets Begin
	m_GlobalDescriptorSets = std::make_unique<Engine::DescriptorSets>(
		vulkanEngine.GetLogicalDevice().GetHandle(),
		m_DescriptorPool->GetHandle(),
		*m_GlobalDescriptorSetLayout.get(),
		1,
		1
	);
	// Global Descriptor Sets End

	// Create pipeline layouts start
	Engine::PipelineLayoutBuilder pipelineLayoutBuilder = Engine::PipelineLayoutBuilder();

	VkPushConstantRange mainPipelinePushConstant = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) };

	m_MainPipelineLayout = pipelineLayoutBuilder.AddDescriptorSetLayout(m_ObjectGPUDataDescriptorSetLayout->GetHandle())
		.AddDescriptorSetLayout(m_GlobalDescriptorSetLayout->GetHandle())
		.AddPushConstant(mainPipelinePushConstant)
		.BuildPipelineLayout(vulkanEngine.GetLogicalDevice().GetHandle());
	// Create pipeline layouts end 

	// Create pipelines start
	Engine::PipelineBuilder pipelineBuilder = Engine::PipelineBuilder();

	std::string shadersPath = "./Shaders/";
	std::string defaultVert = shadersPath + "default_vert.spv";
	//std::string defaultVert = shadersPath + "sinewave_vert.spv";
	std::string texturedFrag = shadersPath + "textured_frag.spv";
	std::string wireframeFrag = shadersPath + "wireframe_frag.spv";
	std::string untexturedFrag = shadersPath + "colored_frag.spv";
	std::string skyboxVert = shadersPath + "skybox_vert.spv";
	std::string skyboxFrag = shadersPath + "skybox_frag.spv";

	std::cout << defaultVert << '\n';
	std::cout << texturedFrag << '\n';
	std::cout << wireframeFrag << '\n';
	std::cout << untexturedFrag << '\n';
	std::cout << skyboxVert << '\n';
	std::cout << skyboxFrag << '\n';

	Assets::VertexShader defaultVertexShader = Assets::VertexShader("Default Vertex Shader", defaultVert);
	Assets::FragmentShader texturedFragmentShader = Assets::FragmentShader("Textured Fragment Shader", texturedFrag);
	Assets::FragmentShader wireframeFragShader = Assets::FragmentShader("Wireframe Fragment Shader", wireframeFrag);
	Assets::FragmentShader coloredFragShader = Assets::FragmentShader("Colored Fragment Shader", untexturedFrag);

	Assets::VertexShader skyboxVertexShader = Assets::VertexShader("Skybox Vertex Shader", skyboxVert);
	Assets::FragmentShader skyboxFragmentShader = Assets::FragmentShader("Skybox Fragment Shader", skyboxFrag);

	m_TexturedPipeline = pipelineBuilder.AddVertexShader(defaultVertexShader)
		.AddFragmentShader(texturedFragmentShader)
		.AddRenderPass(vulkanEngine.GetDefaultRenderPass().GetHandle())
		.AddPipelineLayout(*m_MainPipelineLayout)
		.BuildGraphicsPipeline(vulkanEngine);

	wireframeFragShader.PolygonMode = Assets::FragmentShader::Polygon::LINE;
	wireframeFragShader.LineWidth = 3.0f;

	m_WireframePipeline = pipelineBuilder.AddVertexShader(defaultVertexShader)
		.AddFragmentShader(wireframeFragShader)
		.AddRenderPass(vulkanEngine.GetDefaultRenderPass().GetHandle())
		.AddPipelineLayout(*m_MainPipelineLayout)
		.BuildGraphicsPipeline(vulkanEngine);

	m_ColoredPipeline = pipelineBuilder.AddVertexShader(defaultVertexShader)
		.AddFragmentShader(coloredFragShader)
		.AddRenderPass(vulkanEngine.GetDefaultRenderPass().GetHandle())
		.AddPipelineLayout(*m_MainPipelineLayout)
		.BuildGraphicsPipeline(vulkanEngine);

	m_SkyboxPipeline = pipelineBuilder.AddVertexShader(skyboxVertexShader)
		.AddFragmentShader(skyboxFragmentShader)
		.AddRenderPass(vulkanEngine.GetDefaultRenderPass().GetHandle())
		.AddPipelineLayout(*m_MainPipelineLayout)
		.BuildGraphicsPipeline(vulkanEngine);
	// Create pipelines end 

	m_Ship.SetGraphicsPipeline(m_TexturedPipeline.get());
}

void ModelViewer::CleanUp() {
	m_Ship.ResetResources();

	m_Materials.clear();
	m_Textures.clear();

	m_GPUDataBuffer.reset();
	m_SceneGeometryBuffer.reset();
	m_DescriptorPool.reset();
	m_ObjectGPUDataDescriptorSetLayout.reset();
	m_GlobalDescriptorSetLayout.reset();

	m_MainPipelineLayout.reset();

	m_TexturedPipeline.reset();
	m_WireframePipeline.reset();
	m_ColoredPipeline.reset();
	m_SkyboxPipeline.reset();
}

void ModelViewer::Update(float d, Engine::InputSystem::Input& input) {
	m_Camera->OnUpdate(d, input);
	m_Ship.OnUpdate(d);
}

void RenderSkybox(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	//VkDeviceSize offsets[] = { p_ActiveScene->VertexOffset };
	VkDeviceSize offsets[] = { sizeof(uint32_t) * m_Ship.IndicesAmount };

	vkCmdBindVertexBuffers(
		commandBuffer,
		0,
		1,
		&m_SceneGeometryBuffer->GetBuffer(currentFrame),
		offsets
	);

	vkCmdBindIndexBuffer(
		commandBuffer,
		m_SceneGeometryBuffer->GetBuffer(currentFrame),
		0,
		VK_INDEX_TYPE_UINT32
	);

	m_SceneGPUData.view = m_Camera->ViewMatrix;
	m_SceneGPUData.proj = m_Camera->ProjectionMatrix;

	VkDeviceSize sceneBufferOffset = m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].ChunkSize + m_GPUDataBuffer->Chunks[MATERIAL_BUFFER_INDEX].ChunkSize;
	m_GPUDataBuffer->Update(currentFrame, sceneBufferOffset, &m_SceneGPUData, sizeof(Engine::ApplicationCore::SceneGPUData));

	m_GlobalDescriptorSets->Bind(
		currentFrame,
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_MainPipelineLayout->GetHandle()
	);

	Render(currentFrame, commandBuffer, m_TexturedPipeline->GetHandle());
	Render(currentFrame, commandBuffer, m_ColoredPipeline->GetHandle());

	if (m_Settings.wireframeEnabled)
		Render(currentFrame, commandBuffer, m_WireframePipeline->GetHandle());

	if (m_Settings.renderSkybox && m_Skybox)
		RenderSkybox(commandBuffer, m_SkyboxPipeline->GetHandle());
}

void ModelViewer::Render(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	if (m_Ship.GetGraphicsPipeline()->GetHandle() != graphicsPipeline && graphicsPipeline != m_WireframePipeline->GetHandle())
		return;

	m_Ship.DescriptorSets->Bind(
		currentFrame, 
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		m_MainPipelineLayout->GetHandle()
	);

	Engine::ApplicationCore::ObjectGPUData objectGPUData = Engine::ApplicationCore::ObjectGPUData();
	objectGPUData.model = m_Ship.GetModelMatrix();

	//VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
	VkDeviceSize objectBufferOffset = 0 * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
	m_GPUDataBuffer->Update(currentFrame, objectBufferOffset, &objectGPUData, sizeof(Engine::ApplicationCore::ObjectGPUData));

	for (const auto& mesh : m_Ship.Meshes) {
		vkCmdPushConstants(
			commandBuffer,
			m_MainPipelineLayout->GetHandle(),
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
	ImGui::Begin("Settings");
	ImGui::Text("Last Frame: %f ms", m_Settings.ms);
	ImGui::Text("Framerate: %.1f fps", m_Settings.frames);
	ImGui::Checkbox("Limit Framerate", &m_Settings.limitFramerate);
	ImGui::Checkbox("Enable Wireframe", &m_Settings.wireframeEnabled);
	ImGui::Checkbox("Render SKybox", &m_Settings.renderSkybox);

	m_Camera->OnUIRender();
	m_Ship.OnUIRender();
}

bool ModelViewer::IsDone(Engine::InputSystem::Input& input) {
	return input.Keys[GLFW_KEY_ESCAPE].IsPressed;
}

int main() {
	
	Engine::Settings settings = {};
	settings.Title = "ModelViewer.exe";
	settings.Width = 1600;
	settings.Height = 900;
	settings.uiEnabled = true;
	
	Engine::ApplicationCore app = Engine::ApplicationCore(settings);
	ModelViewer modelViewer = ModelViewer(settings);

	app.RunApplication(modelViewer);

	return 0;
}