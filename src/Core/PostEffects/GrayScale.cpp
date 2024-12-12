#include "GrayScale.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/RenderPassManager.h"
#include "../Core/BufferManager.h"

namespace GrayScale {
	Graphics::Shader m_DefaultVertShader = {};
	Graphics::Shader m_GrayScaleFragShader = {};

	Graphics::PipelineState m_GrayScalePSO = {};
	Graphics::PipelineStateDescription psoDesc = {};

	VkDescriptorSetLayout m_GrayScaleDescriptorLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_GrayScaleSet[Graphics::FRAMES_IN_FLIGHT];

	Graphics::InputLayout m_GrayScaleInputLayout = {
		.pushConstants = {},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		}
	};
	
	bool m_Initialized = false;
}

void GrayScale::Initialize() {
	if (m_Initialized)
		return;

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
	psoDesc.psoInputLayout.push_back(m_GrayScaleInputLayout);
	psoDesc.cullMode = VK_CULL_MODE_FRONT_BIT;
	psoDesc.noVertex = true;

	gfxDevice->CreateDescriptorSetLayout(m_GrayScaleDescriptorLayout, m_GrayScaleInputLayout.bindings);

	m_Initialized = true;
}

void GrayScale::Shutdown() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyShader(m_DefaultVertShader);
	gfxDevice->DestroyShader(m_GrayScaleFragShader);
	gfxDevice->DestroyPipeline(m_GrayScalePSO);
	gfxDevice->DestroyDescriptorSetLayout(m_GrayScaleDescriptorLayout);

	m_Initialized = false;
}

void GrayScale::Render(const VkCommandBuffer& commandBuffer, const Graphics::RenderPass& renderPass) {
	if (!m_Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	if (m_GrayScalePSO.pipeline == VK_NULL_HANDLE || m_GrayScalePSO.renderPass != &renderPass) {
		gfxDevice->CreatePipelineState(psoDesc, m_GrayScalePSO, renderPass);

		for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
			gfxDevice->CreateDescriptorSet(m_GrayScaleDescriptorLayout, m_GrayScaleSet[i]);
		}
	}

	/*
	gfxDevice->WriteDescriptor(
		m_GrayScaleInputLayout.bindings[0], 
		m_GrayScaleSet[gfxDevice->GetCurrentFrameIndex()], 
		Graphics::g_SceneColor);
	*/

	/*
	VkCommandBuffer singleTimeCommandBuffer = gfxDevice->BeginSingleTimeCommandBuffer(gfxDevice->m_CommandPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = gfxDevice->GetSwapChain().swapChainImages[gfxDevice->GetCurrentFrameIndex()];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	vkCmdPipelineBarrier(singleTimeCommandBuffer, sourceStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	gfxDevice->EndSingleTimeCommandBuffer(singleTimeCommandBuffer, gfxDevice->m_CommandPool);

	gfxDevice->WriteDescriptor(
		m_GrayScaleInputLayout.bindings[0],
		m_GrayScaleSet[gfxDevice->GetCurrentFrameIndex()],
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		gfxDevice->GetSwapChain().swapChainImageViews[gfxDevice->GetCurrentFrameIndex()],
		gfxDevice->GetSwapChain().swapChainImageSamplers[gfxDevice->GetCurrentFrameIndex()]);
	*/

	// TODO: maybe find a way to extract the result of a render pass instead of checking msaa


	gfxDevice->WriteDescriptor(
		m_GrayScaleInputLayout.bindings[0], 
		m_GrayScaleSet[gfxDevice->GetCurrentFrameIndex()],
		(gfxDevice->m_MsaaSamples & VK_SAMPLE_COUNT_1_BIT) ? Graphics::g_SceneColor : Graphics::g_ResolvedColor);

	gfxDevice->BindDescriptorSet(
		m_GrayScaleSet[gfxDevice->GetCurrentFrameIndex()], 
		commandBuffer, 
		m_GrayScalePSO.pipelineLayout, 0, 1
	);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GrayScalePSO.pipeline);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}