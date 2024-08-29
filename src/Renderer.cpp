#include "Renderer.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "TextureLoader.h"
#include "ModelLoader.h"
#include "Application.h"
#include "ConstantBuffers.h"
#include "Helper.h"
#include "../Assets/Material.h"

#include <vector>
#include <string>

namespace Renderer {
	bool m_Initialized = false;

	Engine::Graphics::Texture m_Skybox;

	Engine::Graphics::Shader m_SkyboxVertexShader = {};
	Engine::Graphics::Shader m_SkyboxFragShader = {};
	Engine::Graphics::Shader m_DefaultVertShader = {};
	Engine::Graphics::Shader m_ColorFragShader = {};

	Engine::Graphics::Buffer m_SkyboxBuffer = {};
	Engine::Graphics::Buffer m_GlobalDataBuffer = {};
	Engine::Graphics::Buffer m_ModelBuffer = {};

	Engine::Graphics::PipelineState m_SkyboxPSO = {};
	Engine::Graphics::PipelineState m_ColorPSO = {};
	Engine::Graphics::PipelineState m_WireframePSO = {};

	VkDescriptorSetLayout m_ModelDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;

	VkDescriptorSet m_ModelDescriptorSet = VK_NULL_HANDLE;

	GlobalConstants m_GlobalConstants = {};

	std::vector<Material> m_Materials;
	std::vector<Texture> m_Textures;
}

std::shared_ptr<Model> Renderer::LoadModel(const std::string& path) {
	return ModelLoader::LoadModel(path, m_Materials, m_Textures);
}

void Renderer::Init() {
	if (m_Initialized)
		return;

	m_Initialized = true;
}

void Renderer::LoadResources() {
	if (!m_Initialized)
		return;

	Engine::Graphics::GraphicsDevice* gfxDevice = GetDevice();

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

	InputLayout globalInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_Textures.size()), VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		}
	};

	InputLayout modelInputLayout = {
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }
		}
	};

	m_GlobalDataBuffer = gfxDevice->CreateBuffer(sizeof(GlobalConstants));
	m_ModelBuffer = gfxDevice->CreateBuffer(sizeof(ModelConstants));

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

	gfxDevice->CreateDescriptorSetLayout(m_GlobalDescriptorSetLayout, globalInputLayout.bindings);

	for (int i = 0; i < Engine::Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_GlobalDescriptorSetLayout, gfxDevice->GetFrame(i).bindlessSet);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[0], gfxDevice->GetFrame(i).bindlessSet, m_GlobalDataBuffer);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[1], gfxDevice->GetFrame(i).bindlessSet, materialBuffer);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[2], gfxDevice->GetFrame(i).bindlessSet, m_Textures);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[3], gfxDevice->GetFrame(i).bindlessSet, m_Skybox);
	}

	gfxDevice->CreateDescriptorSetLayout(m_ModelDescriptorSetLayout, modelInputLayout.bindings);
	gfxDevice->CreateDescriptorSet(m_ModelDescriptorSetLayout, m_ModelDescriptorSet);
	gfxDevice->WriteDescriptor(modelInputLayout.bindings[0], m_ModelDescriptorSet, m_ModelBuffer);
}

void Renderer::Destroy() {
	Engine::Graphics::GraphicsDevice* gfxDevice = GetDevice();

	for (auto texture : m_Textures) {
		gfxDevice->DestroyImage(texture);
	}

	m_Textures.clear();
	m_Materials.clear();
	
	gfxDevice->DestroyDescriptorSetLayout(m_GlobalDescriptorSetLayout);
	gfxDevice->DestroyDescriptorSetLayout(m_ModelDescriptorSetLayout);
	gfxDevice->DestroyImage(m_Skybox);
	gfxDevice->DestroyShader(m_DefaultVertShader);
	gfxDevice->DestroyShader(m_ColorFragShader);
	gfxDevice->DestroyShader(m_SkyboxVertexShader);
	gfxDevice->DestroyShader(m_SkyboxFragShader);
	gfxDevice->DestroyPipeline(m_ColorPSO);
	gfxDevice->DestroyPipeline(m_SkyboxPSO);
	
	m_Initialized = false;
}

void Renderer::RenderSkybox(const VkCommandBuffer& commandBuffer) {
	Engine::Graphics::GraphicsDevice* gfxDevice = GetDevice();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPSO.pipeline);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void Renderer::UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera) {
	GraphicsDevice* gfxDevice = GetDevice();
	
	m_GlobalConstants.view = camera.ViewMatrix;
	m_GlobalConstants.proj = camera.ProjectionMatrix;

	gfxDevice->UpdateBuffer(m_GlobalDataBuffer, &m_GlobalConstants);
	gfxDevice->BindDescriptorSet(gfxDevice->GetCurrentFrame().bindlessSet, commandBuffer, m_ColorPSO.pipelineLayout, 0, 1);
}

void Renderer::RenderModel(const VkCommandBuffer& commandBuffer, Model& model) {
	GraphicsDevice* gfxDevice = GetDevice();

	VkDeviceSize offsets[] = { sizeof(uint32_t) * model.TotalIndices };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ColorPSO.pipeline);

	gfxDevice->BindDescriptorSet(m_ModelDescriptorSet, commandBuffer, m_ColorPSO.pipelineLayout, 1, 1);

	ModelConstants modelConstant = {};
	modelConstant.model = model.GetModelMatrix();

	gfxDevice->UpdateBuffer(m_ModelBuffer, &modelConstant);

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

void Renderer::RenderWireframe(const VkCommandBuffer& commandBuffer) {

}
