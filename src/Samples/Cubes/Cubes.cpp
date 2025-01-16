#include <iostream>

#include "../../src/Core/Application.h"
#include "../../src/Core/GraphicsDevice.h"
#include "../../src/Core/ConstantBuffers.h"
//#include "../../src/Core/RenderPassManager.h"

#include "../../src/Core/VulkanHeader.h"

#include "../../src/Utils/ModelLoader.h"

#include "../../Assets/Camera.h"
#include "../../Assets/Model.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#define HORIZONTAL_CUBES 11
#define VERTICAL_CUBES 11
#define TOTAL_CUBES HORIZONTAL_CUBES * VERTICAL_CUBES

// TODO: Fix this sample

class Cubes : public Application::IScene {
public:
	Cubes() {
		settings.Title = "Cubes.exe";
		settings.Width = 1600;
		settings.Height = 900;
		settings.uiEnabled = true;
	};

	virtual void StartUp() override;
	virtual void CleanUp() override;
	virtual void Update(float d, InputSystem::Input& input) override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) override;
	virtual void RenderUI() override;
	virtual void Resize(uint32_t width, uint32_t height) override;

private:
	Assets::Camera m_Camera = {};

	uint32_t m_ScreenWidth = settings.Width;
	uint32_t m_ScreenHeight = settings.Height;

	std::vector<std::shared_ptr<Assets::Model>> m_Cubes;

	Graphics::Shader m_VertexShader = {};
	Graphics::Shader m_FragShader = {};

	Graphics::Buffer m_SceneBuffer = {};
	Graphics::Buffer m_ModelBuffer = {};

	Graphics::PipelineState m_PSO = {};

	VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_Set[Graphics::FRAMES_IN_FLIGHT];

	struct PushConstant {
		int index;
	};

	GlobalConstants m_GlobalConstants = {};
};

void Cubes::StartUp() {
	
	glm::vec3 position = glm::vec3(-0.6f, 3.2f, 38.0f);

	float fov = 45.0f;
	float yaw = -94.0f;
	float pitch = -6.0f;

	m_Camera.Init(position, fov, yaw, pitch, m_ScreenWidth, m_ScreenHeight);

	float offset = 2.0f;
	float x = -HORIZONTAL_CUBES;
	float y = -VERTICAL_CUBES;
	float z = 0.0f;

	float rotation_offset = 0.2f;

	for (int i = 0; i < VERTICAL_CUBES; i++) {
		for (int j = 0; j < HORIZONTAL_CUBES; j++) {
			m_Cubes.emplace_back(ModelLoader::LoadModel(ModelType::CUBE));
			m_Cubes[m_Cubes.size() - 1]->Transformations.translation.x = x;
			m_Cubes[m_Cubes.size() - 1]->Transformations.translation.y = y;
			m_Cubes[m_Cubes.size() - 1]->Transformations.translation.z = z;

			x += offset;
		}
		
		x = -HORIZONTAL_CUBES;
		y += offset;
	}

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader, "../src/Samples/Cubes/vertex.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragShader, "../src/Samples/Cubes/color.frag");

	m_SceneBuffer = gfxDevice->CreateBuffer(sizeof(GlobalConstants));
	m_ModelBuffer = gfxDevice->CreateBuffer(sizeof(ModelConstants) * TOTAL_CUBES);

	Graphics::InputLayout inputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL, 0, sizeof(PushConstant) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },			
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },		
		}
	};

	Graphics::PipelineStateDescription desc = {};
	desc.Name = "Cubes";
	desc.vertexShader = &m_VertexShader;
	desc.fragmentShader = &m_FragShader;
	desc.psoInputLayout.push_back(inputLayout);

//	gfxDevice->CreatePipelineState(desc, m_PSO, Graphics::g_ColorRenderPass);
	gfxDevice->CreateDescriptorSetLayout(m_SetLayout, inputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_SetLayout, m_Set[i]);
		gfxDevice->WriteDescriptor(inputLayout.bindings[0], m_Set[i], m_SceneBuffer);
		gfxDevice->WriteDescriptor(inputLayout.bindings[1], m_Set[i], m_ModelBuffer);
	}
}

void Cubes::CleanUp() {
	m_Cubes.clear();
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_FragShader);
	gfxDevice->DestroyDescriptorSetLayout(m_SetLayout);
	gfxDevice->DestroyPipeline(m_PSO);
}

void Cubes::Update(float d, InputSystem::Input& input) {
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_Camera.OnUpdate(d, input);

	m_GlobalConstants.time = d;
	m_GlobalConstants.view = m_Camera.ViewMatrix;
	m_GlobalConstants.proj = m_Camera.ProjectionMatrix;
	m_GlobalConstants.cameraPosition = glm::vec4(m_Camera.Position, 1.0f);

	std::vector<ModelConstants> modelConstants;

	for (uint32_t i = 0; i < m_Cubes.size(); i++) {
		m_Cubes[i]->OnUpdate(d);

		m_Cubes[i]->Transformations.rotation.x += d * 0.021f;
		m_Cubes[i]->Transformations.rotation.y += d * 0.037f;
		
		if (m_Cubes[i]->Transformations.rotation.x > 360.0f)
			m_Cubes[i]->Transformations.rotation.x = 0.0f;

		if (m_Cubes[i]->Transformations.rotation.y > 360.0f)
			m_Cubes[i]->Transformations.rotation.y = 0.0f;

		modelConstants.push_back({});

		modelConstants[i].model = m_Cubes[i]->GetModelMatrix();
		modelConstants[i].normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(modelConstants[i].model))));
		modelConstants[i].flipUvVertically = m_Cubes[i]->FlipUvVertically;
		modelConstants[i].outlineWidth = m_Cubes[i]->OutlineWidth;
	}

	gfxDevice->UpdateBuffer(m_SceneBuffer, &m_GlobalConstants);
	gfxDevice->UpdateBuffer(m_ModelBuffer, modelConstants.data());

}

void Cubes::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->BindDescriptorSet(m_Set[currentFrame], commandBuffer, m_PSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);

	for (int i = 0; i < m_Cubes.size(); i++) {
		VkDeviceSize offsets[] = { sizeof(uint32_t) * m_Cubes[i]->TotalIndices };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Cubes[i]->DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_Cubes[i]->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		PushConstant pushConstant = { .index = i };
		
		for (const auto& mesh : m_Cubes[i]->Meshes) {
			vkCmdPushConstants(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstant), &pushConstant);
			vkCmdDrawIndexed(
				commandBuffer, 
				static_cast<uint32_t>(mesh.Indices.size()), 
				1, 
				static_cast<uint32_t>(mesh.IndexOffset), 
				static_cast<int32_t>(mesh.VertexOffset),
				0);
		}
	}
}

void Cubes::RenderUI() {
	m_Camera.OnUIRender();

	for (int i = 0; i < m_Cubes.size(); i++) {
		m_Cubes[i]->OnUIRender();
	}
}

void Cubes::Resize(uint32_t width, uint32_t height) {
	m_Camera.Resize(width, height);
	m_ScreenWidth = width;
	m_ScreenHeight = height;
}

//RUN_APPLICATION(Cubes);