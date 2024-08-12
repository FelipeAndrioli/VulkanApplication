#include "Renderer.h"
#include "Graphics.h"
#include "GraphicsDevice.h"
#include "TextureLoader.h"

#include <vector>
#include <string>

namespace Renderer {
	bool m_Initialized = false;

	Engine::Graphics::Texture m_Skybox;
	Engine::Graphics::Shader m_SkyboxVertexShader;
	Engine::Graphics::Shader m_SkyboxFragShader;
	Engine::Graphics::GPUBuffer m_SkyboxBuffer;
	Engine::Graphics::PipelineState m_SkyboxPSO;

	VkDescriptorSet m_SkyboxDescriptor;

	SceneGPUData m_SkyboxGPUData = {};
}

void Renderer::Init() {
	if (m_Initialized)
		return;

	Engine::Graphics::GraphicsDevice* gfxDevice = GetDevice();

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

	InputLayout skyboxInputLayout = {
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		}
	};

	Engine::Graphics::BufferDescription bufferDesc = {};
	bufferDesc.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferDesc.BufferSize = sizeof(Renderer::SceneGPUData);
	bufferDesc.MemoryProperty = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	gfxDevice->CreateBuffer(bufferDesc, m_SkyboxBuffer, sizeof(Renderer::SceneGPUData));
	gfxDevice->WriteBuffer(m_SkyboxBuffer, &m_SkyboxGPUData, sizeof(Renderer::SceneGPUData), 0);

	PipelineStateDescription psoDesc = {};
	psoDesc.Name = "Skybox PSO";
	psoDesc.vertexShader = &m_SkyboxVertexShader;
	psoDesc.fragmentShader = &m_SkyboxFragShader;
	psoDesc.cullMode = VK_CULL_MODE_BACK_BIT;
	psoDesc.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	psoDesc.polygonMode = VK_POLYGON_MODE_FILL;
	psoDesc.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	psoDesc.pipelineExtent = gfxDevice->m_SwapChainExtent;
	psoDesc.psoInputLayout.push_back(skyboxInputLayout);

	gfxDevice->CreatePipelineState(psoDesc, m_SkyboxPSO);
	gfxDevice->CreateDescriptorSet(m_SkyboxPSO.descriptorSetLayout[0], m_SkyboxDescriptor);
	gfxDevice->WriteDescriptor(skyboxInputLayout.bindings[0], m_SkyboxDescriptor, m_SkyboxBuffer.Handle, sizeof(Renderer::SceneGPUData), 0);
	gfxDevice->WriteDescriptor(skyboxInputLayout.bindings[1], m_SkyboxDescriptor, m_Skybox);

	m_Initialized = true;
}

void Renderer::Destroy() {
	Engine::Graphics::GraphicsDevice* gfxDevice = GetDevice();
	
	m_Initialized = false;

	gfxDevice->DestroyBuffer(m_SkyboxBuffer);
	gfxDevice->DestroyImage(m_Skybox);
	gfxDevice->DestroyShader(m_SkyboxVertexShader);
	gfxDevice->DestroyShader(m_SkyboxFragShader);
	gfxDevice->DestroyPipeline(m_SkyboxPSO);
}

void Renderer::RenderSkybox(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera) {
	Engine::Graphics::GraphicsDevice* gfxDevice = GetDevice();

	m_SkyboxGPUData.view = camera.ViewMatrix;
	m_SkyboxGPUData.proj = camera.ProjectionMatrix;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SkyboxPSO.pipeline);
	gfxDevice->UpdateBuffer(m_SkyboxBuffer, 0, &m_SkyboxGPUData, sizeof(Renderer::SceneGPUData));
	gfxDevice->BindDescriptorSet(m_SkyboxDescriptor, commandBuffer, m_SkyboxPSO.pipelineLayout, 0, 1);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}
