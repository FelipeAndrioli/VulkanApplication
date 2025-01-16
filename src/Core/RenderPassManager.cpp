#include "RenderPassManager.h"

#include "../Core/BufferManager.h"

namespace Graphics {
	RenderPass g_ColorRenderPass;
	RenderPass g_PostEffectsRenderPass;

	std::unordered_map<VkRenderPass, std::vector<VkFramebuffer>> framebufferMap;
}

void Graphics::InitializeStaticRenderPasses(uint32_t width, uint32_t height) {

	using namespace Graphics;

	GraphicsDevice* gfxDevice = GetDevice();

	// Color Render Pass
	{
		g_ColorRenderPass.description.extent = { width, height };
		g_ColorRenderPass.description.viewport = {
			.x = 0,
			.y = 0,
			.width = static_cast<float>(width),
			.height = static_cast<float>(height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		g_ColorRenderPass.description.scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = width,
				.height = height
			}
		};
		g_ColorRenderPass.description.sampleCount = g_MsaaSceneColor.Description.MsaaSamples;
		g_ColorRenderPass.description.flags = eColorAttachment | eDepthAttachment | eColorLoadOpClear | eColorStoreOpStore;

		if (!(gfxDevice->m_MsaaSamples & VK_SAMPLE_COUNT_1_BIT))
			g_ColorRenderPass.description.flags |= eResolveAttachment;

		gfxDevice->CreateRenderPass(g_ColorRenderPass);
	}

	// Post Effects Render Pass
	{
		g_PostEffectsRenderPass.description.extent = { width, height };
		g_PostEffectsRenderPass.description.viewport = {
			.x = 0,
			.y = 0,
			.width = static_cast<float>(width),
			.height = static_cast<float>(height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		g_PostEffectsRenderPass.description.scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = width,
				.height = height
			}
		};
		g_PostEffectsRenderPass.description.sampleCount = g_PostEffects.Description.MsaaSamples;
		g_PostEffectsRenderPass.description.flags = eColorAttachment | eColorLoadOpClear | eColorStoreOpStore | eFinalLayoutTransferSrc;

		gfxDevice->CreateRenderPass(g_PostEffectsRenderPass);
	}

	// Create framebuffers
	{
		g_ColorRenderPass.framebuffers.resize(gfxDevice->GetSwapChain().swapChainImageViews.size());
		g_PostEffectsRenderPass.framebuffers.resize(gfxDevice->GetSwapChain().swapChainImageViews.size());

		std::vector<VkImageView> colorFramebufferAttachments;

		if (gfxDevice->m_MsaaSamples & VK_SAMPLE_COUNT_1_BIT) {
			colorFramebufferAttachments.emplace_back(g_SceneColor.ImageView);
			colorFramebufferAttachments.emplace_back(g_SceneDepth.ImageView);
		}
		else {
			colorFramebufferAttachments.emplace_back(g_MsaaSceneColor.ImageView);
			colorFramebufferAttachments.emplace_back(g_SceneDepth.ImageView);
			colorFramebufferAttachments.emplace_back(g_SceneColor.ImageView);
		}

		std::vector<VkImageView> postEffectsFramebufferAttachments = { 
			Graphics::g_PostEffects.ImageView 
		};

		for (int i = 0; i < gfxDevice->GetSwapChain().swapChainImageViews.size(); i++) {

			gfxDevice->CreateFramebuffer(
				g_ColorRenderPass.handle, 
				colorFramebufferAttachments, 
				g_ColorRenderPass.description.extent, 
				g_ColorRenderPass.framebuffers[i]);

			gfxDevice->CreateFramebuffer(
				g_PostEffectsRenderPass.handle,
				postEffectsFramebufferAttachments,
				g_PostEffectsRenderPass.description.extent,
				g_PostEffectsRenderPass.framebuffers[i]);
		}
	}
}

void Graphics::ResizeRenderPasses(uint32_t width, uint32_t height) {
	ShutdownRenderPasses();
	InitializeStaticRenderPasses(width, height);
}

void Graphics::ResizeRenderPass(RenderPass& renderPass, uint32_t width, uint32_t height) {

}

void Graphics::ShutdownRenderPasses() {
	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	gfxDevice->DestroyRenderPass(g_ColorRenderPass);
	gfxDevice->DestroyRenderPass(g_PostEffectsRenderPass);
}
