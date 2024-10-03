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
	Graphics::Shader m_LightSourceFragShader = {};
	Graphics::Shader m_LightSourceVertShader = {};
	Graphics::Shader m_OutlineFragShader = {};
	Graphics::Shader m_OutlineVertShader = {};

	Graphics::Buffer m_SkyboxBuffer = {};
	Graphics::Buffer m_GlobalDataBuffer = {};

	Graphics::PipelineState m_SkyboxPSO = {};
	Graphics::PipelineState m_ColorPSO = {};
	Graphics::PipelineState m_ColorStencilPSO = {};
	Graphics::PipelineState m_OutlinePSO = {};
	Graphics::PipelineState m_WireframePSO = {};
	Graphics::PipelineState m_LightSourcePSO = {};
	
	VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;

	GlobalConstants m_GlobalConstants = {};

	std::vector<Material> m_Materials;
	std::vector<Texture> m_Textures;

	float outlineWidth = 0.0f;
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
	gfxDevice->DestroyShader(m_LightSourceFragShader);
	gfxDevice->DestroyShader(m_LightSourceVertShader);
	gfxDevice->DestroyShader(m_OutlineVertShader);
	gfxDevice->DestroyShader(m_OutlineFragShader);
	gfxDevice->DestroyPipeline(m_ColorPSO);
	gfxDevice->DestroyPipeline(m_ColorStencilPSO);
	gfxDevice->DestroyPipeline(m_OutlinePSO);
	gfxDevice->DestroyPipeline(m_SkyboxPSO);
	gfxDevice->DestroyPipeline(m_WireframePSO);
	gfxDevice->DestroyPipeline(m_LightSourcePSO);

	LightManager::Shutdown();

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

	/*
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SkyboxVertexShader, "./Shaders/skybox_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SkyboxFragShader, "./Shaders/skybox_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_DefaultVertShader, "./Shaders/default_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_ColorFragShader, "./Shaders/color_ps.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_WireframeFragShader, "./Shaders/wireframe_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourceVertShader, "./Shaders/light_source_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourceFragShader, "./Shaders/light_source_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_OutlineVertShader, "./Shaders/outline_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_OutlineFragShader, "./Shaders/outline_frag.spv");
	*/

	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SkyboxVertexShader, "../src/Assets/Shaders/skybox.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SkyboxFragShader, "../src/Assets/Shaders/skybox.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_DefaultVertShader, "../src/Assets/Shaders/default.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_ColorFragShader, "../src/Assets/Shaders/color_ps.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_WireframeFragShader, "../src/Assets/Shaders/wireframe.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourceVertShader, "../src/Assets/Shaders/light_source.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourceFragShader, "../src/Assets/Shaders/light_source.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_OutlineVertShader, "../src/Assets/Shaders/outline.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_OutlineFragShader, "../src/Assets/Shaders/outline.frag");

	InputLayout globalInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int) },
			{ VK_SHADER_STAGE_VERTEX_BIT, sizeof(int), sizeof(int) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS },
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS },
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

	PipelineStateDescription colorStencilPSODesc = {};
	colorStencilPSODesc.Name = "Color Stencil Pipeline";
	colorStencilPSODesc.vertexShader = &m_DefaultVertShader;
	colorStencilPSODesc.fragmentShader = &m_ColorFragShader;
	colorStencilPSODesc.pipelineExtent = gfxDevice->GetSwapChainExtent();
	colorStencilPSODesc.psoInputLayout.push_back(globalInputLayout);
	colorStencilPSODesc.psoInputLayout.push_back(modelInputLayout);
	colorStencilPSODesc.cullMode = VK_CULL_MODE_NONE;
	colorStencilPSODesc.stencilTestEnable = true;
	colorStencilPSODesc.stencilState.compareOp = VK_COMPARE_OP_ALWAYS;
	colorStencilPSODesc.stencilState.failOp = VK_STENCIL_OP_REPLACE;
	colorStencilPSODesc.stencilState.depthFailOp = VK_STENCIL_OP_REPLACE;
	colorStencilPSODesc.stencilState.passOp = VK_STENCIL_OP_REPLACE;
	colorStencilPSODesc.stencilState.compareMask = 0xff;
	colorStencilPSODesc.stencilState.writeMask = 0xff;
	colorStencilPSODesc.stencilState.reference = 1;
	
	gfxDevice->CreatePipelineState(colorStencilPSODesc, m_ColorStencilPSO);

	PipelineStateDescription outlinePSODesc = {};
	outlinePSODesc.Name = "Outline Pipeline";
	outlinePSODesc.vertexShader = &m_OutlineVertShader;
	outlinePSODesc.fragmentShader = &m_OutlineFragShader;
	outlinePSODesc.pipelineExtent = gfxDevice->GetSwapChainExtent();
	outlinePSODesc.psoInputLayout.push_back(globalInputLayout);
	outlinePSODesc.psoInputLayout.push_back(modelInputLayout);
	outlinePSODesc.stencilTestEnable = true;
	outlinePSODesc.cullMode = VK_CULL_MODE_NONE;
	outlinePSODesc.stencilState.compareOp = VK_COMPARE_OP_NOT_EQUAL;
	outlinePSODesc.stencilState.failOp = VK_STENCIL_OP_KEEP;
	outlinePSODesc.stencilState.depthFailOp = VK_STENCIL_OP_KEEP;
	outlinePSODesc.stencilState.passOp = VK_STENCIL_OP_REPLACE;
	outlinePSODesc.stencilState.compareMask = 0xff;
	outlinePSODesc.stencilState.writeMask = 0xff;
	outlinePSODesc.stencilState.reference = 1;
	outlinePSODesc.depthTestEnable = true;										// change to false if want to see through walls
	
	gfxDevice->CreatePipelineState(outlinePSODesc, m_OutlinePSO);

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

	PipelineStateDescription lightSourcePSODesc = {};
	lightSourcePSODesc.Name = "Light Source PSO";
	lightSourcePSODesc.vertexShader = &m_LightSourceVertShader;
	lightSourcePSODesc.fragmentShader = &m_LightSourceFragShader;
	lightSourcePSODesc.noVertex = true;
	lightSourcePSODesc.pipelineExtent = gfxDevice->GetSwapChainExtent();
	lightSourcePSODesc.psoInputLayout.push_back(globalInputLayout);
	
	gfxDevice->CreatePipelineState(lightSourcePSODesc, m_LightSourcePSO);

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
	m_GlobalConstants.cameraPosition = glm::vec4(camera.Position, 1.0f);
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

	if (model.RenderOutline) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ColorStencilPSO.pipeline);
		gfxDevice->BindDescriptorSet(model.ModelDescriptorSet, commandBuffer, m_ColorStencilPSO.pipelineLayout, 1, 1);
	} else {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ColorPSO.pipeline);
		gfxDevice->BindDescriptorSet(model.ModelDescriptorSet, commandBuffer, m_ColorPSO.pipelineLayout, 1, 1);
	}

	ModelConstants modelConstant = {};
	modelConstant.model = model.GetModelMatrix();
	modelConstant.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(modelConstant.model))));
	modelConstant.flipUvVertically = model.FlipUvVertically;
	modelConstant.outlineWidth = model.OutlineWidth;

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

void Renderer::RenderOutline(const VkCommandBuffer& commandBuffer, Assets::Model& model) {
	GraphicsDevice* gfxDevice = GetDevice();

	VkDeviceSize offsets[] = { sizeof(uint32_t) * model.TotalIndices };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_OutlinePSO.pipeline);

	gfxDevice->BindDescriptorSet(model.ModelDescriptorSet, commandBuffer, m_OutlinePSO.pipelineLayout, 1, 1);

	ModelConstants modelConstant = {};
	modelConstant.model = model.GetModelMatrix();
	modelConstant.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(modelConstant.model))));
	modelConstant.flipUvVertically = model.FlipUvVertically;
	modelConstant.outlineWidth = model.OutlineWidth;

	gfxDevice->UpdateBuffer(model.ModelBuffer, &modelConstant);

	for (const auto& mesh : model.Meshes) {
		vkCmdPushConstants(
			commandBuffer,
			m_OutlinePSO.pipelineLayout,
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

void Renderer::RenderLightSources(const VkCommandBuffer& commandBuffer) {
	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightSourcePSO.pipeline);

	for (int i = 0; i < LightManager::GetLights().size(); i++) {

		LightData light = LightManager::GetLights().at(i);

		if (light.type == LightType::Directional)
			continue;

		vkCmdPushConstants(
			commandBuffer, 
			m_LightSourcePSO.pipelineLayout, 
			VK_SHADER_STAGE_VERTEX_BIT,
			sizeof(int),
			sizeof(int),
			&i
		);

		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}
}
