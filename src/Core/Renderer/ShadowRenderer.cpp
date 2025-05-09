#include "ShadowRenderer.h"

#include "../Assets/Model.h"

#include "../RenderTarget.h"

void ShadowRenderer::StartUp() {
	LoadResources();
}

void ShadowRenderer::CleanUp() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyPipeline(m_PSO);
	gfxDevice->DestroyShader(m_VertexShader);
	gfxDevice->DestroyShader(m_GeometryShader);
	gfxDevice->DestroyShader(m_FragmentShader);
}

void ShadowRenderer::Update(const float d, const float c, const InputSystem::Input& input) {

}

void ShadowRenderer::Render(const VkCommandBuffer& commandBuffer) {

}

void ShadowRenderer::RenderUI() {

}

const Graphics::GPUImage& ShadowRenderer::GetDepthBuffer() {
	return m_RenderTarget->GetDepthBuffer();
}

void ShadowRenderer::SetExtent(uint32_t width, uint32_t height) {
	m_Width		= width;
	m_Height	= height;

}

void ShadowRenderer::SetPrecision(uint32_t precision) {
	m_Precision = precision;
}

void ShadowRenderer::Render(const VkCommandBuffer& commandBuffer, const std::vector<std::shared_ptr<Assets::Model>>& models, const glm::mat4& lightViewProj) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_ShadowMappingGPUData[0].Light = lightViewProj;

	m_RenderTarget->Begin(commandBuffer);

	for (int i = 0; i < models.size(); i++) {
		m_ModelGPUData[i].Model = models[i]->GetModelMatrix();
	}

	gfxDevice->UpdateBuffer		(m_ShadowMappingUBO[gfxDevice->GetCurrentFrameIndex()], m_ShadowMappingGPUData.data());
	gfxDevice->UpdateBuffer		(m_ModelUBO[gfxDevice->GetCurrentFrameIndex()],			m_ModelGPUData.data());
	gfxDevice->BindDescriptorSet(m_Set, commandBuffer, m_PSO.pipelineLayout, 0, 1);
			
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);
	
	for (int i = 0; i < models.size(); i++) {
		
		std::shared_ptr<Assets::Model> model = models[i];
	
		VkDeviceSize offsets[] = { sizeof(uint32_t) * model->TotalIndices };

		vkCmdBindVertexBuffers	(commandBuffer, 0, 1, &model->DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer	(commandBuffer, model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		m_PushConstants.ModelIndex = model->ModelIndex;
		m_PushConstants.ActiveLightSources = 1;

		vkCmdPushConstants(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants), &m_PushConstants);

		for (int j = 0; j < model->Meshes.size(); j++) {
			const Assets::Mesh* mesh = &model->Meshes[j];

			if (mesh->PSOFlags & PSOFlags::tTransparent)
				continue;

			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->Indices.size()), 1, static_cast<uint32_t>(mesh->IndexOffset), static_cast<int32_t>(mesh->VertexOffset), 0);
		}
	}

	m_RenderTarget->End(commandBuffer);
}

void ShadowRenderer::Render(const VkCommandBuffer& commandBuffer, const std::vector<std::shared_ptr<Assets::Model>>& models, const std::vector<Scene::LightComponent>& lights) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (int i = 0; i < lights.size(); i++) {
		m_ShadowMappingGPUData[i].Light = lights[i].viewProj;
	}

	m_RenderTarget->Begin(commandBuffer);

	for (int i = 0; i < models.size(); i++) {
		m_ModelGPUData[i].Model = models[i]->GetModelMatrix();
	}

	gfxDevice->UpdateBuffer		(m_ShadowMappingUBO[gfxDevice->GetCurrentFrameIndex()], m_ShadowMappingGPUData.data());
	gfxDevice->UpdateBuffer		(m_ModelUBO[gfxDevice->GetCurrentFrameIndex()],			m_ModelGPUData.data());
	gfxDevice->BindDescriptorSet(m_Set, commandBuffer, m_PSO.pipelineLayout, 0, 1);
			
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);
	
	for (int i = 0; i < models.size(); i++) {
		
		std::shared_ptr<Assets::Model> model = models[i];
	
		VkDeviceSize offsets[] = { sizeof(uint32_t) * model->TotalIndices };

		vkCmdBindVertexBuffers	(commandBuffer, 0, 1, &model->DataBuffer.Handle, offsets);
		vkCmdBindIndexBuffer	(commandBuffer, model->DataBuffer.Handle, 0, VK_INDEX_TYPE_UINT32);

		m_PushConstants.ModelIndex = model->ModelIndex;
		m_PushConstants.ActiveLightSources = (int)lights.size();

		vkCmdPushConstants(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants), &m_PushConstants);

		for (int j = 0; j < model->Meshes.size(); j++) {
			const Assets::Mesh* mesh = &model->Meshes[j];
			
			if (mesh->PSOFlags & PSOFlags::tTransparent)
				continue;

			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->Indices.size()), 1, static_cast<uint32_t>(mesh->IndexOffset), static_cast<int32_t>(mesh->VertexOffset), 0);
		}
	}

	m_RenderTarget->End(commandBuffer);
}

void ShadowRenderer::LoadResources() {
	m_RenderTarget = std::make_unique<Graphics::DepthOnlyRenderTarget>(m_Width, m_Height, m_Precision, m_Layers);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

#ifdef RUNTIME_SHADER_COMPILATION
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		"../src/Assets/Shaders/shadow_mapping.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_GEOMETRY_BIT, m_GeometryShader,	"../src/Assets/Shaders/shadow_mapping.geom");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	"../src/Assets/Shaders/shadow_mapping.frag");
#else
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		"../src/Assets/Shaders/shadow_mapping_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_GEOMETRY_BIT, m_GeometryShader,	"../src/Assets/Shaders/shadow_mapping_geom.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	"../src/Assets/Shaders/shadow_mapping_frag.spv");
#endif

	m_PSOInputLayout = {
		.pushConstants = {
			{ VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants) }
		},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT },	// Light Matrix Array
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT },	// Model Matrix Array 
		}
	};

	m_PSODesc.Name				= "Shadow Mapping";
	m_PSODesc.vertexShader		= &m_VertexShader;
	m_PSODesc.geometryShader	= &m_GeometryShader;
	m_PSODesc.fragmentShader	= &m_FragmentShader;
	m_PSODesc.cullMode			= VK_CULL_MODE_FRONT_BIT;
	m_PSODesc.psoInputLayout	.push_back(m_PSOInputLayout);

	gfxDevice->CreatePipelineState(m_PSODesc, m_PSO, *m_RenderTarget);
	gfxDevice->CreateDescriptorSet(m_PSO.descriptorSetLayout, m_Set);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		m_ModelUBO[i]			= gfxDevice->CreateBuffer(sizeof(ModelGPUData) * MAX_MODELS);
		m_ShadowMappingUBO[i]	= gfxDevice->CreateBuffer(sizeof(ShadowMappingGPUData) * MAX_LIGHT_SOURCES);

		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[0], m_Set, m_ShadowMappingUBO[i]);
		gfxDevice->WriteDescriptor(m_PSOInputLayout.bindings[1], m_Set, m_ModelUBO[i]);
	}

}
