#include "RenderPassManager.h"

#include "BufferManager.h"

namespace Graphics {
	RenderPass g_ColorRenderPass;
	RenderPass g_PostEffectsRenderPass;
	RenderPass g_DebugRenderPass;
//	RenderPass g_FinalRenderPass;
}

void Graphics::InitializeStaticRenderPasses(uint32_t width, uint32_t height) {
	Graphics::GraphicsDevice* gfxDevice = GetDevice();

	// Color Render Pass
	{	
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		Graphics::RenderPassDesc desc = {};
		desc.scissor.offset = { 0, 0 };
		desc.scissor.extent = { width, height };
		desc.extent = { width, height };
		desc.viewport.x = 0.0f;
		desc.viewport.y = 0.0f;
		desc.viewport.width = static_cast<float>(width);
		desc.viewport.height = static_cast<float>(height);
		desc.viewport.minDepth = 0.0f;
		desc.viewport.maxDepth = 1.0f;
		desc.sampleCount = Graphics::g_SceneColor.Description.MsaaSamples;

		g_ColorRenderPass.description = desc;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = gfxDevice->GetSwapChain().swapChainImageFormat;
		colorAttachment.samples = Graphics::g_SceneColor.Description.MsaaSamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		g_ColorRenderPass.description.clearValues.push_back({ .color {0.0f, 0.0f, 0.0f, 1.0f} });
		
		g_ColorRenderPass.attachments.emplace_back(colorAttachment);

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = g_ColorRenderPass.attachments.size() - 1;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency colorDependency = {};
		colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		colorDependency.dstSubpass = 0;
		colorDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		colorDependency.srcAccessMask = VK_ACCESS_NONE_KHR;
		colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		colorDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		g_ColorRenderPass.dependencies.emplace_back(colorDependency);

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = gfxDevice->GetDepthFormat();
		depthAttachment.samples = Graphics::g_SceneDepth.Description.MsaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		g_ColorRenderPass.description.clearValues.push_back({ .depthStencil { 1.0f, 0 } });
		
		g_ColorRenderPass.attachments.emplace_back(depthAttachment);

		VkAttachmentReference depthAttachmentReference = {};
		depthAttachmentReference.attachment = g_ColorRenderPass.attachments.size() - 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		subpass.pDepthStencilAttachment = &depthAttachmentReference;
		
		VkSubpassDependency depthDependency = {};
		depthDependency.srcSubpass = 0;
		depthDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		depthDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		depthDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		depthDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		depthDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		g_ColorRenderPass.dependencies.emplace_back(depthDependency);

		if (!(gfxDevice->m_MsaaSamples & VK_SAMPLE_COUNT_1_BIT)) {
			VkAttachmentDescription resolveAttachment = {};
			resolveAttachment.format = gfxDevice->GetSwapChain().swapChainImageFormat;
			resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			g_ColorRenderPass.description.clearValues.push_back({ .color {0.0f, 0.0f, 0.0f, 1.0f} });
			
			g_ColorRenderPass.attachments.emplace_back(resolveAttachment);

			VkSubpassDependency resolveDependency = {};
			resolveDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			resolveDependency.dstSubpass = 0;
			resolveDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			resolveDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			resolveDependency.srcAccessMask = 0;
			resolveDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			resolveDependency.dependencyFlags = 0;

			VkAttachmentReference resolveAttachmentRef = {};
			resolveAttachmentRef.attachment = g_ColorRenderPass.attachments.size() - 1;
			resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			subpass.pResolveAttachments = &resolveAttachmentRef;

			g_ColorRenderPass.dependencies.emplace_back(resolveDependency);
		}
			
		g_ColorRenderPass.subpasses.emplace_back(subpass);

		gfxDevice->CreateRenderPass(g_ColorRenderPass);
	}

	// Post Effects Render Pass
	{
		Graphics::RenderPassDesc desc = {};
		desc.scissor.offset = { 0, 0 };
		desc.scissor.extent = { width, height };
		desc.extent = { width, height };
		desc.viewport.x = 0.0f;
		desc.viewport.y = 0.0f;
		desc.viewport.width = static_cast<float>(width);
		desc.viewport.height = static_cast<float>(height);
		desc.viewport.minDepth = 0.0f;
		desc.viewport.maxDepth = 1.0f;
		desc.clearValues.push_back({ .color = {0.0f, 0.0f, 0.0f, 1.0f} });
		desc.sampleCount = VK_SAMPLE_COUNT_1_BIT;

		g_PostEffectsRenderPass.description = desc;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = gfxDevice->GetSwapChain().swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		g_PostEffectsRenderPass.attachments.emplace_back(colorAttachment);

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = g_PostEffectsRenderPass.attachments.size() - 1;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		g_PostEffectsRenderPass.subpasses.emplace_back(subpass);

		VkSubpassDependency colorDependency = {};
		colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		colorDependency.dstSubpass = 0;
		colorDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		colorDependency.srcAccessMask = VK_ACCESS_NONE_KHR;
		colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		colorDependency.dependencyFlags = 0;

		g_PostEffectsRenderPass.dependencies.emplace_back(colorDependency);

		gfxDevice->CreateRenderPass(g_PostEffectsRenderPass);
	}

	// Debug Render Pass
	{
		Graphics::RenderPassDesc desc = {};
		desc.scissor.offset = { 0, 0 };
		desc.scissor.extent = { width, height };
		desc.extent = { width, height };
		desc.viewport.x = 0.0f;
		desc.viewport.y = 0.0f;
		desc.viewport.width = static_cast<float>(width);
		desc.viewport.height = static_cast<float>(height);
		desc.viewport.minDepth = 0.0f;
		desc.viewport.maxDepth = 1.0f;
		desc.clearValues.push_back({ .color = {0.0f, 0.0f, 0.0f, 1.0f} });
		desc.sampleCount = VK_SAMPLE_COUNT_1_BIT;

		g_DebugRenderPass.description = desc;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = gfxDevice->GetSwapChain().swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		g_DebugRenderPass.attachments.emplace_back(colorAttachment);

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = g_DebugRenderPass.attachments.size() - 1;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		g_DebugRenderPass.subpasses.emplace_back(subpass);

		VkSubpassDependency colorDependency = {};
		colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		colorDependency.dstSubpass = 0;
		colorDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		colorDependency.srcAccessMask = VK_ACCESS_NONE_KHR;
		colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		colorDependency.dependencyFlags = 0;

		g_DebugRenderPass.dependencies.emplace_back(colorDependency);

		gfxDevice->CreateRenderPass(g_DebugRenderPass);
	}

	g_ColorRenderPass.framebuffers.resize(gfxDevice->GetSwapChain().swapChainImageViews.size());
	g_PostEffectsRenderPass.framebuffers.resize(gfxDevice->GetSwapChain().swapChainImageViews.size());
	g_DebugRenderPass.framebuffers.resize(gfxDevice->GetSwapChain().swapChainImageViews.size());

	for (int i = 0; i < gfxDevice->GetSwapChain().swapChainImageViews.size(); i++) {

		std::vector<VkImageView> colorFramebufferAttachments = {
			Graphics::g_SceneColor.ImageView,
			Graphics::g_SceneDepth.ImageView,
		};

		if (!(gfxDevice->m_MsaaSamples & VK_SAMPLE_COUNT_1_BIT)) {
			colorFramebufferAttachments.emplace_back(Graphics::g_ResolvedColor.ImageView);
		}

		std::vector<VkImageView> postEffectsFramebufferAttachments = { 
			Graphics::g_PostEffects.ImageView 
		};

		std::vector<VkImageView> debugFramebufferAttachments = { gfxDevice->GetSwapChain().swapChainImageViews[i] };

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

		gfxDevice->CreateFramebuffer(
			g_DebugRenderPass.handle,
			debugFramebufferAttachments,
			g_DebugRenderPass.description.extent,
			g_DebugRenderPass.framebuffers[i]);
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
	gfxDevice->DestroyRenderPass(g_DebugRenderPass);
}
