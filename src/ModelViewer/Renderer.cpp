#include "Renderer.h"

#include <vector>
#include <string>

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/Application.h"
#include "../Core/ConstantBuffers.h"
#include "../Core/ResourceManager.h"

#include "../Utils/TextureLoader.h"
#include "../Utils/ModelLoader.h"
#include "../Utils/Helper.h"

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
	Graphics::Shader m_TransparentFragShader = {};
	Graphics::Shader m_DepthFragShader = {};
	Graphics::Shader m_NormalsFragShader = {};

	Graphics::Buffer m_ModelBuffer = {};
	Graphics::Buffer m_SkyboxBuffer = {};
	Graphics::Buffer m_CamerasBuffer[Graphics::FRAMES_IN_FLIGHT] = {};
	Graphics::Buffer m_GlobalDataBuffer[Graphics::FRAMES_IN_FLIGHT] = {};

	Graphics::PipelineState m_SkyboxPSO = {};
	Graphics::PipelineState m_ColorPSO = {};
	Graphics::PipelineState m_ColorStencilPSO = {};
	Graphics::PipelineState m_OutlinePSO = {};
	Graphics::PipelineState m_WireframePSO = {};
	Graphics::PipelineState m_LightSourcePSO = {};
	Graphics::PipelineState m_TransparentPSO = {};
	Graphics::PipelineState m_TransparentStencilPSO = {};
	Graphics::PipelineState m_RenderDepthPSO = {};
	Graphics::PipelineState m_RenderNormalsPSO= {};

	VkPipelineLayout m_GlobalPipelineLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;

	GlobalConstants m_GlobalConstants = {};

	std::array<std::shared_ptr<Assets::Model>, MAX_MODELS> m_Models;
	
	uint32_t m_TotalModels = 0;

	int m_CameraIndex = 0;
}

std::shared_ptr<Assets::Model> Renderer::LoadModel(const std::string& path) {
	if (m_TotalModels == MAX_MODELS)
		return nullptr;

	m_Models[m_TotalModels++] = ModelLoader::LoadModel(path);

	uint32_t modelIdx = m_TotalModels - 1;

	m_Models[modelIdx]->ModelIndex = modelIdx;

	return m_Models[modelIdx];
}

void Renderer::Init() {
	if (m_Initialized)
		return;

	m_Initialized = true;
}

void Renderer::Shutdown() {
	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	m_Models.fill(nullptr);
	
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
	gfxDevice->DestroyShader(m_TransparentFragShader);
	gfxDevice->DestroyShader(m_DepthFragShader);
	gfxDevice->DestroyShader(m_NormalsFragShader);

	gfxDevice->DestroyPipeline(m_ColorPSO);
	gfxDevice->DestroyPipeline(m_ColorStencilPSO);
	gfxDevice->DestroyPipeline(m_OutlinePSO);
	gfxDevice->DestroyPipeline(m_SkyboxPSO);
	gfxDevice->DestroyPipeline(m_WireframePSO);
	gfxDevice->DestroyPipeline(m_LightSourcePSO);
	gfxDevice->DestroyPipeline(m_TransparentPSO);
	gfxDevice->DestroyPipeline(m_TransparentStencilPSO);
	gfxDevice->DestroyPipeline(m_RenderDepthPSO);
	gfxDevice->DestroyPipeline(m_RenderNormalsPSO);

	gfxDevice->DestroyPipelineLayout(m_GlobalPipelineLayout);

	LightManager::Shutdown();

	m_Initialized = false;
}

void Renderer::LoadResources(const Graphics::IRenderTarget& renderTarget) {
	if (!m_Initialized)
		return;

	LightManager::Init();

	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	ResourceManager* rm = ResourceManager::Get();

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

#ifdef RUNTIME_SHADER_COMPILATION
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SkyboxVertexShader, "../src/Assets/Shaders/skybox.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SkyboxFragShader, "../src/Assets/Shaders/skybox.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_DefaultVertShader, "../src/Assets/Shaders/default.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_ColorFragShader, "../src/Assets/Shaders/color_ps.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_WireframeFragShader, "../src/Assets/Shaders/wireframe.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourceVertShader, "../src/Assets/Shaders/light_source.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourceFragShader, "../src/Assets/Shaders/light_source.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_OutlineVertShader, "../src/Assets/Shaders/outline.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_OutlineFragShader, "../src/Assets/Shaders/outline.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_TransparentFragShader, "../src/Assets/Shaders/transparent_ps.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_DepthFragShader, "../src/Assets/Shaders/depth.frag");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_NormalsFragShader, "../src/Assets/Shaders/debug_normals.frag");
#else 
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_SkyboxVertexShader, "./Shaders/skybox_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_SkyboxFragShader, "./Shaders/skybox_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_DefaultVertShader, "./Shaders/default_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_ColorFragShader, "./Shaders/color_ps.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_WireframeFragShader, "./Shaders/wireframe_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_LightSourceVertShader, "./Shaders/light_source_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_LightSourceFragShader, "./Shaders/light_source_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_OutlineVertShader, "./Shaders/outline_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_OutlineFragShader, "./Shaders/outline_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_TransparentFragShader, "./Shaders/transparent_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_DepthFragShader, "./Shaders/depth_frag.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_NormalsFragShader, "./Shaders/debug_normals_frag.spv");
#endif

	InputLayout globalInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PipelinePushConstants) },
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS },
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(rm->GetTextures().size()), VK_SHADER_STAGE_FRAGMENT_BIT},
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS },
			{ 6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS }		// Multiple cameras UBO 
		}
	};

	m_ModelBuffer = gfxDevice->CreateBuffer(sizeof(ModelConstants) * MAX_MODELS);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		m_CamerasBuffer[i] = gfxDevice->CreateBuffer(sizeof(CameraConstants) * MAX_CAMERAS);
		m_GlobalDataBuffer[i] = gfxDevice->CreateBuffer(sizeof(GlobalConstants));
		gfxDevice->WriteSubBuffer(m_GlobalDataBuffer[i], &m_GlobalConstants, sizeof(GlobalConstants));
	}

	gfxDevice->CreateDescriptorSetLayout(m_GlobalDescriptorSetLayout, globalInputLayout.bindings);
	gfxDevice->CreatePipelineLayout(m_GlobalDescriptorSetLayout, m_GlobalPipelineLayout, globalInputLayout.pushConstants);
	
	PipelineStateDescription colorPSODesc = {};
	colorPSODesc.Name = "Color Pipeline";
	colorPSODesc.vertexShader = &m_DefaultVertShader;
	colorPSODesc.fragmentShader = &m_ColorFragShader;
	colorPSODesc.psoInputLayout.push_back(globalInputLayout);

	gfxDevice->CreatePipelineState(colorPSODesc, m_ColorPSO, renderTarget);

	PipelineStateDescription colorStencilPSODesc = {};
	colorStencilPSODesc.Name = "Color Stencil Pipeline";
	colorStencilPSODesc.vertexShader = &m_DefaultVertShader;
	colorStencilPSODesc.fragmentShader = &m_ColorFragShader;
	colorStencilPSODesc.psoInputLayout.push_back(globalInputLayout);
	colorStencilPSODesc.cullMode = VK_CULL_MODE_NONE;
	colorStencilPSODesc.stencilTestEnable = true;
	colorStencilPSODesc.stencilState.compareOp = VK_COMPARE_OP_ALWAYS;
	colorStencilPSODesc.stencilState.failOp = VK_STENCIL_OP_REPLACE;
	colorStencilPSODesc.stencilState.depthFailOp = VK_STENCIL_OP_REPLACE;
	colorStencilPSODesc.stencilState.passOp = VK_STENCIL_OP_REPLACE;
	colorStencilPSODesc.stencilState.compareMask = 0xff;
	colorStencilPSODesc.stencilState.writeMask = 0xff;
	colorStencilPSODesc.stencilState.reference = 1;
	
	gfxDevice->CreatePipelineState(colorStencilPSODesc, m_ColorStencilPSO, renderTarget);

	PipelineStateDescription outlinePSODesc = {};
	outlinePSODesc.Name = "Outline Pipeline";
	outlinePSODesc.vertexShader = &m_OutlineVertShader;
	outlinePSODesc.fragmentShader = &m_OutlineFragShader;
	outlinePSODesc.psoInputLayout.push_back(globalInputLayout);
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
	
	gfxDevice->CreatePipelineState(outlinePSODesc, m_OutlinePSO, renderTarget);

	PipelineStateDescription skyboxPSODesc = {};
	skyboxPSODesc.Name = "Skybox PSO";
	skyboxPSODesc.vertexShader = &m_SkyboxVertexShader;
	skyboxPSODesc.fragmentShader = &m_SkyboxFragShader;
	skyboxPSODesc.noVertex = true;
	skyboxPSODesc.psoInputLayout.push_back(globalInputLayout);

	gfxDevice->CreatePipelineState(skyboxPSODesc, m_SkyboxPSO, renderTarget);

	PipelineStateDescription wireframePSODesc = {};
	wireframePSODesc.Name = "Wireframe PSO";
	wireframePSODesc.vertexShader = &m_DefaultVertShader;
	wireframePSODesc.fragmentShader = &m_WireframeFragShader;
	wireframePSODesc.lineWidth = 2.0f;
	wireframePSODesc.polygonMode = VK_POLYGON_MODE_LINE;
	wireframePSODesc.psoInputLayout.push_back(globalInputLayout);

	gfxDevice->CreatePipelineState(wireframePSODesc, m_WireframePSO, renderTarget);

	PipelineStateDescription lightSourcePSODesc = {};
	lightSourcePSODesc.Name = "Light Source PSO";
	lightSourcePSODesc.vertexShader = &m_LightSourceVertShader;
	lightSourcePSODesc.fragmentShader = &m_LightSourceFragShader;
	lightSourcePSODesc.noVertex = true;
	lightSourcePSODesc.psoInputLayout.push_back(globalInputLayout);
	
	gfxDevice->CreatePipelineState(lightSourcePSODesc, m_LightSourcePSO, renderTarget);

	PipelineStateDescription transparentPSODesc = {};
	transparentPSODesc.Name = "Transparent PSO";
	transparentPSODesc.vertexShader = &m_DefaultVertShader;
	transparentPSODesc.fragmentShader = &m_TransparentFragShader;
	transparentPSODesc.cullMode = VK_CULL_MODE_NONE;
	transparentPSODesc.psoInputLayout.push_back(globalInputLayout);
	transparentPSODesc.colorBlendingEnable = true;
	transparentPSODesc.colorBlendingDesc.blendEnable = VK_TRUE;
	transparentPSODesc.colorBlendingDesc.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	transparentPSODesc.colorBlendingDesc.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	transparentPSODesc.colorBlendingDesc.colorBlendOp = VK_BLEND_OP_ADD;
	transparentPSODesc.colorBlendingDesc.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	transparentPSODesc.colorBlendingDesc.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	transparentPSODesc.colorBlendingDesc.alphaBlendOp = VK_BLEND_OP_ADD;
	
	gfxDevice->CreatePipelineState(transparentPSODesc, m_TransparentPSO, renderTarget);

	PipelineStateDescription transparentStencilPSODesc = {};
	transparentStencilPSODesc.Name = "Transparent Stencil Pipeline";
	transparentStencilPSODesc.vertexShader = &m_DefaultVertShader;
	transparentStencilPSODesc.fragmentShader = &m_TransparentFragShader;
	transparentStencilPSODesc.psoInputLayout.push_back(globalInputLayout);
	transparentStencilPSODesc.cullMode = VK_CULL_MODE_NONE;
	transparentStencilPSODesc.stencilTestEnable = true;
	transparentStencilPSODesc.stencilState.compareOp = VK_COMPARE_OP_ALWAYS;
	transparentStencilPSODesc.stencilState.failOp = VK_STENCIL_OP_REPLACE;
	transparentStencilPSODesc.stencilState.depthFailOp = VK_STENCIL_OP_REPLACE;
	transparentStencilPSODesc.stencilState.passOp = VK_STENCIL_OP_REPLACE;
	transparentStencilPSODesc.stencilState.compareMask = 0xff;
	transparentStencilPSODesc.stencilState.writeMask = 0xff;
	transparentStencilPSODesc.stencilState.reference = 1;

	gfxDevice->CreatePipelineState(transparentPSODesc, m_TransparentStencilPSO, renderTarget);

	PipelineStateDescription renderDepthPSODesc = {};
	renderDepthPSODesc.Name = "Render Depth";
	renderDepthPSODesc.vertexShader = &m_DefaultVertShader;
	renderDepthPSODesc.fragmentShader = &m_DepthFragShader;
	renderDepthPSODesc.psoInputLayout.push_back(globalInputLayout);
	
	m_RenderDepthPSO.description = renderDepthPSODesc;

	PipelineStateDescription renderNormalsPSODesc = {};
	renderNormalsPSODesc.Name = "Render Normals";
	renderNormalsPSODesc.vertexShader = &m_DefaultVertShader;
	renderNormalsPSODesc.fragmentShader = &m_NormalsFragShader;
	renderNormalsPSODesc.psoInputLayout.push_back(globalInputLayout);

	m_RenderNormalsPSO.description = renderNormalsPSODesc;


//	gfxDevice->CreatePipelineState(renderDepthPSODesc, m_RenderDepthPSO, renderTarget);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_GlobalDescriptorSetLayout, gfxDevice->GetFrame(i).bindlessSet);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[0], gfxDevice->GetFrame(i).bindlessSet, m_GlobalDataBuffer[i]);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[1], gfxDevice->GetFrame(i).bindlessSet, rm->GetMaterialBuffer());
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[2], gfxDevice->GetFrame(i).bindlessSet, LightManager::GetLightBuffer());
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[3], gfxDevice->GetFrame(i).bindlessSet, rm->GetTextures());
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[4], gfxDevice->GetFrame(i).bindlessSet, m_Skybox);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[5], gfxDevice->GetFrame(i).bindlessSet, m_ModelBuffer);
		gfxDevice->WriteDescriptor(globalInputLayout.bindings[6], gfxDevice->GetFrame(i).bindlessSet, m_CamerasBuffer[i]);
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

void Renderer::UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const std::array<Assets::Camera, MAX_CAMERAS> cameras) {
	GraphicsDevice* gfxDevice = GetDevice();
	
	m_GlobalConstants.totalLights = LightManager::GetTotalLights();

	gfxDevice->UpdateBuffer(m_GlobalDataBuffer[gfxDevice->GetCurrentFrameIndex()], &m_GlobalConstants);

	LightManager::UpdateBuffer();

	std::array<ModelConstants, MAX_MODELS> modelConstants;

	for (size_t i = 0; i < m_TotalModels; i++) {
		ModelConstants modelConstant = {};
		modelConstant.model = m_Models[i]->GetModelMatrix();
		modelConstant.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(modelConstant.model))));
		modelConstant.flipUvVertically = m_Models[i]->FlipUvVertically;
		modelConstant.outlineWidth = m_Models[i]->OutlineWidth;

		modelConstants[i] = modelConstant;
	}

	gfxDevice->UpdateBuffer(m_ModelBuffer, modelConstants.data());

	std::array<CameraConstants, MAX_CAMERAS> cameraConstants;

	for (size_t i = 0; i < cameras.size(); i++) {
		CameraConstants cameraConstant = {};
		cameraConstant.position = glm::vec4(cameras[i].Position, 1.0f);
		cameraConstant.view = cameras[i].ViewMatrix;
		cameraConstant.proj = cameras[i].ProjectionMatrix;

		cameraConstants[i] = cameraConstant;
	}

	gfxDevice->UpdateBuffer(m_CamerasBuffer[gfxDevice->GetCurrentFrameIndex()], cameraConstants.data());

	gfxDevice->BindDescriptorSet(gfxDevice->GetCurrentFrame().bindlessSet, commandBuffer, m_GlobalPipelineLayout, 0, 1);
}

void Renderer::RenderOutline(const VkCommandBuffer& commandBuffer, Assets::Model& model) {
	GraphicsDevice* gfxDevice = GetDevice();

	VkDeviceSize offsets[] = { sizeof(uint32_t) * model.TotalIndices };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.DataBuffer.Handle, offsets);
	vkCmdBindIndexBuffer(commandBuffer, model.DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_OutlinePSO.pipeline);

	for (const auto& mesh : model.Meshes) {
		PipelinePushConstants pushConstants = {
			.MaterialIdx = static_cast<int>(mesh.MaterialIndex),
			.ModelIdx = static_cast<int>(model.ModelIndex),
			.CameraIdx = m_CameraIndex
		};

		vkCmdPushConstants(
			commandBuffer,
			m_OutlinePSO.pipelineLayout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			0,
			sizeof(PipelinePushConstants),
			&pushConstants
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

	for (const auto& mesh : model.Meshes) {
		PipelinePushConstants pushConstants = {
			.MaterialIdx = static_cast<int>(mesh.MaterialIndex),
			.ModelIdx = static_cast<int>(model.ModelIndex),
			.CameraIdx = m_CameraIndex
		};

		vkCmdPushConstants(
			commandBuffer,
			m_WireframePSO.pipelineLayout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			0,
			sizeof(PipelinePushConstants),
			&pushConstants
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

		PipelinePushConstants pushConstants = {
			.LightSourceIdx = i,
			.CameraIdx = m_CameraIndex
		};

		vkCmdPushConstants(
			commandBuffer,
			m_LightSourcePSO.pipelineLayout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			0,
			sizeof(PipelinePushConstants),
			&pushConstants
		);

		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}
}

void Renderer::SetCameraIndex(int index) {
	m_CameraIndex = index;
}

const Graphics::PipelineState& Renderer::GetPSO(uint16_t flags) {

	if (flags & PSOFlags::tOpaque && flags & PSOFlags::tStencilTest) {
		return m_ColorStencilPSO;
	}

	if (flags & PSOFlags::tTransparent && flags & PSOFlags::tStencilTest) {
		return m_TransparentStencilPSO;
	}

	if (flags & PSOFlags::tTransparent) {
		return m_TransparentPSO;
	}
	
	return m_ColorPSO;
}

const Assets::Camera& Renderer::MeshSorter::GetCamera() {
	return *m_Camera;
}

void Renderer::MeshSorter::SetCamera(const Assets::Camera& camera) {
	m_Camera = &camera;
}

void Renderer::MeshSorter::AddMesh(const Assets::Mesh& mesh, float distance, uint32_t modelIndex, uint32_t totalIndices, Graphics::GPUBuffer& buffer) {
	
	SortKey key = {};
	key.value = m_SortMeshes.size();
	key.distance = distance;

	if (mesh.PSOFlags & PSOFlags::tTransparent) {
		key.key = ~static_cast<uint64_t>(distance);
		key.passId = DrawPass::tTransparent;
		m_PassCounts[DrawPass::tTransparent]++;
	} else {
		key.key = static_cast<uint64_t>(distance);
		key.passId = DrawPass::tOpaque;
		m_PassCounts[DrawPass::tOpaque]++;
	}

	m_SortKeys.push_back(key);
	m_SortMeshes.push_back({ &mesh, &buffer, distance, modelIndex, totalIndices });
}

void Renderer::MeshSorter::Sort() {
	struct { bool operator()(SortKey& a, SortKey& b) const { return a.key < b.key; } } cmp;
	std::sort(m_SortKeys.begin(), m_SortKeys.end(), cmp);
}

void Renderer::MeshSorter::RenderMeshes(const VkCommandBuffer& commandBuffer, DrawPass pass) {

	GraphicsDevice* gfxDevice = GetDevice();

	for (; m_CurrentPass <= pass; m_CurrentPass = (DrawPass)(m_CurrentPass + 1)) {

		const uint32_t passCount = m_PassCounts[m_CurrentPass];

		if (passCount == 0)
			continue;

		const PipelineState* pipeline = nullptr;
		const VkBuffer* geometryBuffer = nullptr;

		const uint32_t lastDraw = m_CurrentDraw + passCount;

		while (m_CurrentDraw < lastDraw) {
			const SortKey& key = m_SortKeys[m_CurrentDraw];
			const SortMesh& sortMesh = m_SortMeshes[key.value];
			const Assets::Mesh& mesh = *sortMesh.mesh;

			const PipelineState* newMeshPipeline = &GetPSO(mesh.PSOFlags);

			if (pipeline == nullptr || newMeshPipeline != pipeline) {
				pipeline = newMeshPipeline;
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
				assert(pipeline != nullptr);
			}

			if (geometryBuffer == nullptr || &sortMesh.bufferPtr->Handle != geometryBuffer) {
				geometryBuffer = &sortMesh.bufferPtr->Handle;

				VkDeviceSize offsets[] = { sizeof(uint32_t) * sortMesh.totalIndices };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, geometryBuffer, offsets);
				vkCmdBindIndexBuffer(commandBuffer, *geometryBuffer, 0, VK_INDEX_TYPE_UINT32);
			}

			assert(geometryBuffer != nullptr);

			PipelinePushConstants pushConstants = {
				.MaterialIdx = static_cast<int>(mesh.MaterialIndex),
				.ModelIdx = static_cast<int>(sortMesh.modelIndex),
				.CameraIdx = m_CameraIndex
			};

			vkCmdPushConstants(
				commandBuffer,
				pipeline->pipelineLayout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				0,
				sizeof(PipelinePushConstants),
				&pushConstants
			);

			vkCmdDrawIndexed(
				commandBuffer,
				static_cast<uint32_t>(mesh.Indices.size()),
				1,
				static_cast<uint32_t>(mesh.IndexOffset),
				static_cast<int32_t>(mesh.VertexOffset),
				0
			);

			++m_CurrentDraw;
		}
	}
}

void Renderer::MeshSorter::RenderMeshes(const VkCommandBuffer& commandBuffer, DrawPass pass, Graphics::IRenderTarget& renderTarget, Graphics::PipelineState* pso) {

	GraphicsDevice* gfxDevice = GetDevice();
	
	for (; m_CurrentPass <= pass; m_CurrentPass = (DrawPass)(m_CurrentPass + 1)) {

		const uint32_t passCount = m_PassCounts[m_CurrentPass];

		if (passCount == 0)
			continue;

		if (pso->renderPass == nullptr 
			|| pso->renderPass->Handle == VK_NULL_HANDLE 
			|| pso->renderPass->Handle != renderTarget.GetRenderPass().Handle) {

			gfxDevice->DestroyPipeline(*pso);
			gfxDevice->CreatePipelineState(pso->description, *pso, renderTarget);
		}

		const PipelineState* pipeline = nullptr;
		const VkBuffer* geometryBuffer = nullptr;

		const uint32_t lastDraw = m_CurrentDraw + passCount;

		while (m_CurrentDraw < lastDraw) {
			const SortKey& key = m_SortKeys[m_CurrentDraw];
			const SortMesh& sortMesh = m_SortMeshes[key.value];
			const Assets::Mesh& mesh = *sortMesh.mesh;

			if (pipeline == nullptr || pso != pipeline) {
				pipeline = pso;
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
				assert(pipeline != nullptr);
			}
		
			if (geometryBuffer == nullptr || &sortMesh.bufferPtr->Handle != geometryBuffer) {
				geometryBuffer = &sortMesh.bufferPtr->Handle;

				VkDeviceSize offsets[] = { sizeof(uint32_t) * sortMesh.totalIndices };

				vkCmdBindVertexBuffers(commandBuffer, 0, 1, geometryBuffer, offsets);
				vkCmdBindIndexBuffer(commandBuffer, *geometryBuffer, 0, VK_INDEX_TYPE_UINT32);
			}

			assert(geometryBuffer != nullptr);

			PipelinePushConstants pushConstants = {
				.MaterialIdx = static_cast<int>(mesh.MaterialIndex),
				.ModelIdx = static_cast<int>(sortMesh.modelIndex),
				.CameraIdx = m_CameraIndex
			};

			vkCmdPushConstants(
				commandBuffer,
				pipeline->pipelineLayout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				0,
				sizeof(PipelinePushConstants),
				&pushConstants
			);

			vkCmdDrawIndexed(
				commandBuffer,
				static_cast<uint32_t>(mesh.Indices.size()),
				1,
				static_cast<uint32_t>(mesh.IndexOffset),
				static_cast<int32_t>(mesh.VertexOffset),
				0
			);

			++m_CurrentDraw;
		}
	}
}

