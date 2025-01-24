#include "GrayScale.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/BufferManager.h"
#include "../Core/RenderTarget.h"

#include "./PostEffects.h"

namespace GrayScale {
	Graphics::Shader m_DefaultVertShader = {};
	Graphics::Shader m_GrayScaleFragShader = {};

	Graphics::PipelineState m_GrayScalePSO = {};
	Graphics::PipelineStateDescription psoDesc = {};
}

void GrayScale::Initialize() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

#ifdef RUNTIME_SHADER_COMPILATION
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_DefaultVertShader, "../src/Assets/Shaders/quad.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_GrayScaleFragShader, "../src/Assets/Shaders/grayscale.frag");
#else
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT, m_DefaultVertShader, "./Shaders/quad_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_GrayScaleFragShader, "./Shaders/grayscale_frag.spv");
#endif

	psoDesc.Name = "Gray Scale Pipeline";
	psoDesc.vertexShader = &m_DefaultVertShader;
	psoDesc.fragmentShader = &m_GrayScaleFragShader;
	psoDesc.psoInputLayout.push_back(PostEffects::m_InputLayout);
	psoDesc.cullMode = VK_CULL_MODE_FRONT_BIT;
	psoDesc.noVertex = true;
}

void GrayScale::Shutdown() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyShader(m_DefaultVertShader);
	gfxDevice->DestroyShader(m_GrayScaleFragShader);
	gfxDevice->DestroyPipeline(m_GrayScalePSO);
}

void GrayScale::Render(const VkCommandBuffer& commandBuffer, const Graphics::IRenderTarget& renderTarget) {

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	if (m_GrayScalePSO.pipeline == VK_NULL_HANDLE || m_GrayScalePSO.renderPass != &renderTarget.GetRenderPass()) {
		gfxDevice->CreatePipelineState(psoDesc, m_GrayScalePSO, renderTarget);
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GrayScalePSO.pipeline);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}