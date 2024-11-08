#include "GrayScale.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/BufferManager.h"

namespace GrayScale {
	Graphics::Shader m_DefaultVertShader = {};
	Graphics::Shader m_GrayScaleFragShader = {};

	Graphics::PipelineState m_GrayScalePSO = {};
	Graphics::PipelineStateDescription psoDesc = {};

//	Graphics::RenderPass renderPass = {};

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

	Graphics::RenderPassDesc desc = {};
	desc.scissor.offset = { 0, 0 };
	desc.scissor.extent = gfxDevice->GetSwapChain().swapChainExtent;
	desc.viewport.x = 0.0f;
	desc.viewport.y = 0.0f;
	desc.viewport.width = static_cast<float>(gfxDevice->GetSwapChain().swapChainExtent.width);
	desc.viewport.height = static_cast<float>(gfxDevice->GetSwapChain().swapChainExtent.height);
	desc.viewport.minDepth = 0.0f;
	desc.viewport.maxDepth = 1.0f;
	desc.clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	desc.clearValues[1].depthStencil = { 1.0f, 0 };
	desc.sampleCount = gfxDevice->m_MsaaSamples;
	desc.flags = Graphics::RenderPass::tColorAttachment | Graphics::RenderPass::tDepthAttachment;

	//gfxDevice->CreateRenderPass(desc, renderPass);
	CreateRenderPass();

	m_Initialized = true;
}

void GrayScale::CreateRenderPass() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	std::vector<VkAttachmentDescription> attachments = {};
	std::vector<VkSubpassDependency> dependencies = {};
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	Graphics::RenderPassDesc desc = {};
	desc.scissor.offset = { 0, 0 };
	desc.scissor.extent = gfxDevice->GetSwapChain().swapChainExtent;
	desc.extent = gfxDevice->GetSwapChain().swapChainExtent;
	desc.viewport.x = 0.0f;
	desc.viewport.y = 0.0f;
	desc.viewport.width = static_cast<float>(gfxDevice->GetSwapChain().swapChainExtent.width);
	desc.viewport.height = static_cast<float>(gfxDevice->GetSwapChain().swapChainExtent.height);
	desc.viewport.minDepth = 0.0f;
	desc.viewport.maxDepth = 1.0f;
	desc.clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	desc.clearValues[1].depthStencil = { 1.0f, 0 };
	desc.sampleCount = Graphics::g_PostEffects.Description.MsaaSamples;

	//renderPass.description = desc;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = Graphics::g_PostEffects.Description.Format;
	//colorAttachment.samples = renderPass.description.sampleCount;
	colorAttachment.samples = Graphics::g_PostEffects.Description.MsaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	attachments.emplace_back(colorAttachment);

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = attachments.size() - 1;
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

	dependencies.emplace_back(colorDependency);

	/*
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = gfxDevice->GetDepthFormat();
	depthAttachment.samples = renderPass.description.sampleCount;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments.emplace_back(depthAttachment);

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = attachments.size() - 1;
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

	dependencies.emplace_back(depthDependency);
	*/

	std::vector<VkSubpassDescription> subpasses = { subpass };

//	gfxDevice->CreateRenderPass(renderPass.handle, attachments, subpasses, dependencies);

	/*
	renderPass.framebuffers.resize(gfxDevice->GetSwapChain().swapChainImageViews.size());

	for (int i = 0; i < gfxDevice->GetSwapChain().swapChainImageViews.size(); i++) {
		std::vector<VkImageView> framebufferAttachments = {};
		framebufferAttachments.emplace_back(Graphics::g_PostEffects.ImageView);
//		framebufferAttachments.emplace_back(Graphics::g_SceneDepth.ImageView);

		gfxDevice->CreateFramebuffer(
			renderPass.handle, 
			framebufferAttachments, 
			renderPass.description.extent, 
			renderPass.framebuffers[i]);
	}
	*/
}

void GrayScale::Shutdown() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	//gfxDevice->DestroyRenderPass(renderPass);
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
			gfxDevice->WriteDescriptor(
				m_GrayScaleInputLayout.bindings[0], 
				m_GrayScaleSet[i], 
				Graphics::g_SceneColor
			);
		}
	}

	gfxDevice->BeginRenderPass(renderPass, commandBuffer);

	gfxDevice->BindDescriptorSet(
		m_GrayScaleSet[gfxDevice->GetCurrentFrameIndex()], 
		commandBuffer, 
		m_GrayScalePSO.pipelineLayout, 0, 1
	);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GrayScalePSO.pipeline);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);

	gfxDevice->EndRenderPass(commandBuffer);
}