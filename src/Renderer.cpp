#include "Renderer.h"

#include <vector>
#include <string>

#include "./Core/Graphics.h"
#include "./Core/GraphicsDevice.h"
#include "./Core/Application.h"
#include "./Core/ConstantBuffers.h"

#include "./Utils/TextureLoader.h"
#include "./Utils/ModelLoader.h"
#include "./Utils/Helper.h"

#include "../Assets/Material.h"
#include "../Assets/Model.h"
#include "../Assets/Camera.h"

#include "LightManager.h"

namespace Renderer {
	bool m_Initialized = false;

	Graphics::Texture m_Skybox;

	Graphics::Shader m_SkyboxVertexShader = {};
	Graphics::Shader m_SkyboxFragShader = {};
	Graphics::Shader m_DefaultVertShader = {};
	Graphics::Shader m_ColorFragShader = {};
	Graphics::Shader m_WireframeFragShader = {};

	Graphics::Buffer m_SkyboxBuffer = {};
	Graphics::Buffer m_GlobalDataBuffer = {};

	Graphics::PipelineState m_SkyboxPSO = {};
	Graphics::PipelineState m_ColorPSO = {};
	Graphics::PipelineState m_WireframePSO = {};
	
	VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;

	GlobalConstants m_GlobalConstants = {};

	std::vector<Material> m_Materials;
	std::vector<Texture> m_Textures;
}

std::shared_ptr<Assets::Model> Renderer::LoadModel(const std::string& path) {
	return ModelLoader::LoadModel(path, m_Materials, m_Textures);
}

void Renderer::Init() {
	if (m_Initialized)
		return;

	m_Initialized = true;
}

void Renderer::Shutdown() {
	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	for (auto texture : m_Textures) {
		gfxDevice->DestroyImage(texture);
	}

	m_Textures.clear();
	m_Materials.clear();
	
	gfxDevice->DestroyDescriptorSetLayout(m_GlobalDescriptorSetLayout);
	gfxDevice->DestroyImage(m_Skybox);
	gfxDevice->DestroyShader(m_DefaultVertShader);
	gfxDevice->DestroyShader(m_ColorFragShader);
	gfxDevice->DestroyShader(m_WireframeFragShader);
	gfxDevice->DestroyShader(m_SkyboxVertexShader);
	gfxDevice->DestroyShader(m_SkyboxFragShader);
	gfxDevice->DestroyPipeline(m_ColorPSO);
	gfxDevice->DestroyPipeline(m_SkyboxPSO);
	gfxDevice->DestroyPipeline(m_WireframePSO);
	
	m_Initialized = false;
}

void Renderer::LoadResources() {
	if (!m_Initialized)
		return;

	LightManager::Init();

	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	// Skybox PSO
	std::vector<std::string> cubeTextures = {
		"./Textures/right.jpg",
		"./Textures/left.jpg",
		"./Textures/top.jpg",
		"./Textures/bottom.jpg",
		"./Textures/front.jpg",
		"./Textures/back.jpg",
	};

	m_Skybox = TextureLoader::LoadCubemapTexture("./Textures/immenstadter_horn_2k.hdr");
	
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SkyboxVertexShader, "./Shaders/skybox_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SkyboxFragShader, "./Shaders/skybox_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_DefaultVertShader, "./Shaders/default_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_ColorFragShader, "./Shaders/color_ps.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_WireframeFragShader, "./Shaders/wireframe_frag.spv");

	InputLayout globalInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS },
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_Textures.size()), VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		}
	};

	InputLayout modelInputLayout = {
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }
		}
	};

	m_GlobalDataBuffer = gfxDevice->CreateBuffer(sizeof(GlobalConstants));

	gfxDevice->WriteBuffer(m_GlobalDataBuffer, &m_GlobalConstants);

	std::vector<MaterialData> meshMaterialData;

	for (const auto& material : m_Materials) {
		meshMaterialData.push_back(material.MaterialData);
	}

	Buffer materialBuffer = gfxDevice->CreateBuffer(sizeof(MaterialData) * m_Materials.size());

	gfxDevice->WriteBuffer(materialBuffer, meshMaterialData.data());

	PipelineStateDescription colorPSODesc = {};
	colorPSODesc.Name = "Color Pipeline";
	colorPSODesc.vertexShader = &m_DefaultVertShader;
	colorPSODesc.fragmentShader = &m_ColorFragShader;
	colorPSODesc.pipelineExtent = gfxDevice->GetSwapChainExtent();
	colorPSODesc.psoInputLayout.push_back(globalInputLayout);
	colorPSODesc.psoInputLayout.push_back(modelInputLayout);
	
	gfxDevice->CreatePipelineState(colorPSODesc, m_ColorPSO);
	
	PipelineStateDescription skyboxPSODesc = {};
	skyboxPSODesc.Name = "Skybox PSO";
	skyboxPSODesc.vertexShader = &m_SkyboxVertexShader;
	skyboxPSODesc.fragmentShader = &m_SkyboxFragShader;
	skyboxPSODesc.noVertex = true;
	skyboxPSODesc.pipelineExtent = gfxDevice->GetSwapChainExtent();
	skyboxPSODesc.psoInputLayout.push_back(globalInputLayout);

	gfxDevice->CreatePipelineState(skyboxPSODesc, m_SkyboxPSO);

	PipelineStateDescription wireframePSODesc = {};
	wireframePSODesc.Name = "Wireframe PSO";
	wireframePSODesc.vertexShader = &m_DefaultVertShader;
	wireframePSODesc.fragmentShader = &m_WireframeFragShader;
	wireframePSODesc.lineWidth = 2.0f;
	wireframePSODesc.polygonMode = VK_POLYGON_MODE_LINE;
	wireframePSODesc.pipelineExtent = gfxDevice->GetSwapChainExtent();
	wireframePSODesc.psoInputLayout.push_back(globalInputLayout);
	wireframePSODesc.psoInputLayout.push_back(modelInputLayout);

	gfxDevice->CreatePipelineState(wireframePSODesc, m_WireframePSO);

	gfxDevice->CreateDescriptorSetLayout(m_GlobalDescriptorSetLayout, globalInputLayout.bindings);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_GlobalDescriptorSetLayout, gfxDevice->GetFrame(i).bindlessSet);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[0], gfxDevice->GetFrame(i).bindlessSet, m_GlobalDataBuffer);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[1], gfxDevice->GetFrame(i).bindlessSet, materialBuffer);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[2], gfxDevice->GetFrame(i).bindlessSet, LightManager::GetLightBuffer());
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[3], gfxDevice->GetFrame(i).bindlessSet, m_Textures);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[4], gfxDevice->GetFrame(i).bindlessSet, m_Skybox);
	}
}

void Renderer::OnUIRender() {
	LightManager::OnUIRender();
}

void Renderer::RenderSkybox(const VkCommandBuffer& commandBuffer) {
	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPSO.pipeline);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void Renderer::UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera) {
	GraphicsDevice* gfxDevice = GetDevice();
	
	m_GlobalConstants.view = camera.ViewMatrix;
	m_GlobalConstants.proj = camera.ProjectionMatrix;
	m_GlobalConstants.totalLights = LightManager::GetTotalLights();

	gfxDevice->UpdateBuffer(m_GlobalDataBuffer, &m_GlobalConstants);

	LightManager::UpdateBuffer();

	gfxDevice->BindDescriptorSet(gfxDevice->GetCurrentFrame().bindlessSet, commandBuffer, m_ColorPSO.pipelineLayout, 0, 1);
}

void Renderer::RenderModel(const VkCommandBuffer& commandBuffer, Assets::Model& model) {
	GraphicsDevice* gfxDevice = GetDevice();

	VkDeviceSize offsets[] = { sizeof(uint32_t) * model.TotalIndices };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ColorPSO.pipeline);

	gfxDevice->BindDescriptorSet(model.ModelDescriptorSet, commandBuffer, m_ColorPSO.pipelineLayout, 1, 1);

	ModelConstants modelConstant = {};
	modelConstant.model = model.GetModelMatrix();
	modelConstant.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(modelConstant.model))));
	modelConstant.flipUvVertically = model.FlipUvVertically;

	gfxDevice->UpdateBuffer(model.ModelBuffer, &modelConstant);

	for (const auto& mesh : model.Meshes) {
		vkCmdPushConstants(
			commandBuffer,
			m_ColorPSO.pipelineLayout,
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

void Renderer::RenderWireframe(const VkCommandBuffer& commandBuffer, Assets::Model& model) {
	GraphicsDevice* gfxDevice = GetDevice();

	VkDeviceSize offsets[] = { sizeof(uint32_t) * model.TotalIndices };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_WireframePSO.pipeline);

	gfxDevice->BindDescriptorSet(model.ModelDescriptorSet, commandBuffer, m_WireframePSO.pipelineLayout, 1, 1);

	ModelConstants modelConstant = {};
	modelConstant.model = model.GetModelMatrix();

	gfxDevice->UpdateBuffer(model.ModelBuffer, &modelConstant);

	for (const auto& mesh : model.Meshes) {
		vkCmdPushConstants(
			commandBuffer,
			m_WireframePSO.pipelineLayout,
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
