#include "RenderTarget.h"

namespace Graphics {
	/* ========================== Interface Render Target Implementation Begin ========================== */

	IRenderTarget::~IRenderTarget() {
		Destroy();
	}

	void IRenderTarget::Destroy() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->DestroyRenderPass(m_RenderPass);

		for (int i = 0; i < m_TotalImages; i++) {
			gfxDevice->DestroyImage(m_Images[i]);
		}

		gfxDevice->DestroyFramebuffer(m_Framebuffers);
	}

	void IRenderTarget::Resize(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		m_RenderPass.Description.viewport.width = width;
		m_RenderPass.Description.viewport.height = height;
		m_RenderPass.Description.extent = GetExtent();
		m_RenderPass.Description.scissor.extent = GetExtent();

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->DestroyFramebuffer(m_Framebuffers);

		if (m_ColorIndex != -1) {
			gfxDevice->ResizeImage(m_Images[m_ColorIndex], width, height);
			gfxDevice->CreateImageSampler(m_Images[m_ColorIndex]);
		}

		if (m_ResolveIndex != -1) {
			gfxDevice->ResizeImage(m_Images[m_ResolveIndex], width, height);
			gfxDevice->CreateImageSampler(m_Images[m_ResolveIndex]);
		}

		if (m_DepthIndex != -1) {
			gfxDevice->ResizeImage(m_Images[m_DepthIndex], width, height);
		}
	}

	void IRenderTarget::BeginRenderPass(const VkCommandBuffer& commandBuffer) {

		if (m_Started)
			return;

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = m_RenderPass.Handle;
		renderPassBeginInfo.framebuffer = m_Framebuffers[gfxDevice->GetSwapChain().ImageIndex];
		renderPassBeginInfo.renderArea.offset = m_RenderPass.Description.offset;
		renderPassBeginInfo.renderArea.extent = m_RenderPass.Description.extent;
		renderPassBeginInfo.pNext = nullptr;

		if (m_RenderPass.Description.clearValues.size() == 0) {
			std::array<VkClearValue, 3> clearValues = {};
			int clearValuesCount = 0;

			if (m_RenderPass.Description.flags & eColorAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };
			if (m_RenderPass.Description.flags & eDepthAttachment)
				clearValues[clearValuesCount++] = { .depthStencil{ 1.0f, 0 } };
			if (m_RenderPass.Description.flags & eResolveAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };

			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValuesCount);
			renderPassBeginInfo.pClearValues = clearValues.data();
		}
		else {
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(m_RenderPass.Description.clearValues.size());
			renderPassBeginInfo.pClearValues = m_RenderPass.Description.clearValues.data();
		}

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = m_RenderPass.Description.viewport;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = m_RenderPass.Description.scissor;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		m_Started = true;
	}

	void IRenderTarget::EndRenderPass(const VkCommandBuffer& commandBuffer) {
		if (!m_Started)
			return;

		vkCmdEndRenderPass(commandBuffer);

		m_Started = false;
	}

	VkFramebuffer& IRenderTarget::GetFramebuffer(int imageIndex) {
		if (imageIndex < 0 || imageIndex > m_Framebuffers.size())
			return m_Framebuffers[0];

		return m_Framebuffers[imageIndex];
	}

	/* ========================== Interface Render Target Implementation End ========================== */

	/* ========================== SwapChain Render Target Implementation Begin ========================== */

	SwapChainRenderTarget::SwapChainRenderTarget(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		Create();
	}

	void SwapChainRenderTarget::Create() {

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		// Create swap chain render pass
		m_RenderPass.Description.extent = GetExtent();
		m_RenderPass.Description.viewport = {
			.x = 0,
			.y = 0,
			.width = static_cast<float>(m_Width),
			.height = static_cast<float>(m_Height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		m_RenderPass.Description.scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = m_Width,
				.height = m_Height
			}
		};
		m_RenderPass.Description.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		m_RenderPass.Description.flags = eColorAttachment
			| eDepthAttachment
			| eColorLoadOpLoad
			| eColorStoreOpStore
			| eInitialLayoutColorOptimal
			| eFinalLayoutPresent;

		gfxDevice->CreateRenderPass(m_RenderPass);

		m_DepthIndex = m_TotalImages++;
		gfxDevice->CreateDepthBuffer(m_Images[m_DepthIndex], GetExtent(), VK_SAMPLE_COUNT_1_BIT);

		for (int i = 0; i < m_Framebuffers.size(); i++) {

			std::vector<VkImageView> framebufferAttachments = {
				gfxDevice->GetSwapChain().ImageViews[i],
				m_Images[m_DepthIndex].ImageView
			};

			gfxDevice->CreateFramebuffer(
				m_RenderPass.Handle,
				framebufferAttachments,
				GetExtent(),
				m_Framebuffers[i]);
		}
	}

	void SwapChainRenderTarget::CopyColor(const Graphics::GPUImage& colorBuffer, int positionX, int positionY) {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->TransitionImageLayout(
			gfxDevice->GetSwapChain().Images[gfxDevice->GetSwapChain().ImageIndex],
			m_ImageLayout,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		m_ImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		VkImageCopy imageCopy = {};
		imageCopy.extent.width = colorBuffer.Description.Width;
		imageCopy.extent.height = colorBuffer.Description.Height;
		imageCopy.extent.depth = 1;
		imageCopy.srcOffset = { 0, 0, 0 };
		imageCopy.srcSubresource = {
			.aspectMask = colorBuffer.Description.AspectFlags,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = colorBuffer.Description.LayerCount
		};
		imageCopy.dstOffset = { 
			.x = positionX, 
			.y = positionY, 
			.z = 0 
		};
		imageCopy.dstSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		VkCommandBuffer singleTimeCommandBuffer = gfxDevice->BeginSingleTimeCommandBuffer(gfxDevice->m_CommandPool);

		vkCmdCopyImage(
			singleTimeCommandBuffer,
			colorBuffer.Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			gfxDevice->GetSwapChain().Images[gfxDevice->GetSwapChain().ImageIndex],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopy);

		gfxDevice->EndSingleTimeCommandBuffer(singleTimeCommandBuffer, gfxDevice->m_CommandPool);

		gfxDevice->TransitionImageLayout(
			gfxDevice->GetSwapChain().Images[gfxDevice->GetSwapChain().ImageIndex],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		);
		
		m_ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	void SwapChainRenderTarget::Begin(const VkCommandBuffer& commandBuffer) {
		BeginRenderPass(commandBuffer);
	}
	
	void SwapChainRenderTarget::End(const VkCommandBuffer& commandBuffer) {
		EndRenderPass(commandBuffer);
		m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	/* ========================== SwapChain Render Target Implementation End ========================== */

	/* ========================== Offscreen Render Target Implementation Begin ========================== */

	OffscreenRenderTarget::OffscreenRenderTarget(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		Create();
	}

	void OffscreenRenderTarget::Create() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_RenderPass.Description.extent = GetExtent();
		m_RenderPass.Description.viewport = {
			.x = 0, 
			.y = 0,
			.width = static_cast<float>(m_Width),
			.height = static_cast<float>(m_Height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		m_RenderPass.Description.scissor = {
			.offset = {
				.x = 0, 
				.y = 0 
			},
			.extent = {
				.width = m_Width,
				.height = m_Height
			}
		};
		m_RenderPass.Description.sampleCount = gfxDevice->m_MsaaSamples;
		m_RenderPass.Description.flags = eColorAttachment | eDepthAttachment | eColorLoadOpClear | eColorStoreOpStore;

		if (!(gfxDevice->m_MsaaSamples & VK_SAMPLE_COUNT_1_BIT))
			m_RenderPass.Description.flags |= eResolveAttachment;

		gfxDevice->CreateRenderPass(m_RenderPass);

		if (m_RenderPass.Description.flags & eResolveAttachment) {
			m_ResolveIndex = m_TotalImages++;

			gfxDevice->CreateRenderTarget(
				m_Images[m_ResolveIndex],
				gfxDevice->GetSwapChain().ImageFormat,
				GetExtent(),
				gfxDevice->m_MsaaSamples);
			gfxDevice->CreateImageSampler(m_Images[m_ResolveIndex]);
		}

		m_DepthIndex = m_TotalImages++;
		gfxDevice->CreateDepthBuffer(m_Images[m_DepthIndex], GetExtent(), gfxDevice->m_MsaaSamples);

		m_ColorIndex = m_TotalImages++;
		gfxDevice->CreateRenderTarget(
			m_Images[m_ColorIndex],
			gfxDevice->GetSwapChain().ImageFormat,
			GetExtent(),
			VK_SAMPLE_COUNT_1_BIT);

		gfxDevice->CreateImageSampler(m_Images[m_ColorIndex]);

		std::vector<VkImageView> views = {};

		if (m_RenderPass.Description.flags & eResolveAttachment) {
			views.emplace_back(m_Images[m_ResolveIndex].ImageView);
		}

		views.emplace_back(m_Images[m_DepthIndex].ImageView);
		views.emplace_back(m_Images[m_ColorIndex].ImageView);

		for (int i = 0; i < m_Framebuffers.size(); i++) {
			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, views, GetExtent(), m_Framebuffers[i]);
		}
	}

	void OffscreenRenderTarget::ChangeLayout(VkImageLayout newLayout) {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->TransitionImageLayout(m_Images[m_ColorIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, newLayout);
	}

	/* ========================== Offscreen Render Target Implementation End ========================== */

	/* ========================== Post Effects Render Target Implementation Begin ======================== */

	PostEffectsRenderTarget::PostEffectsRenderTarget(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		Create();
	}

	void PostEffectsRenderTarget::Create() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_RenderPass.Description.extent = GetExtent();
		m_RenderPass.Description.viewport = {
			.x = 0,
			.y = 0,
			.width = static_cast<float>(m_Width),
			.height = static_cast<float>(m_Height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		m_RenderPass.Description.scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = m_Width,
				.height = m_Height
			}
		};
		m_RenderPass.Description.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		m_RenderPass.Description.flags = eColorAttachment | eColorLoadOpClear | eColorStoreOpStore | eFinalLayoutTransferSrc;

		gfxDevice->CreateRenderPass(m_RenderPass);

		m_ColorIndex = m_TotalImages++;
		gfxDevice->CreateRenderTarget(
			m_Images[m_ColorIndex],
			gfxDevice->GetSwapChain().ImageFormat,
			GetExtent(),
			VK_SAMPLE_COUNT_1_BIT);

		gfxDevice->CreateImageSampler(m_Images[m_ColorIndex]);

		std::vector<VkImageView> views = { m_Images[m_ColorIndex].ImageView };

		for (int i = 0; i < m_Framebuffers.size(); i++) {
			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, views, GetExtent(), m_Framebuffers[i]);
		}
	}

	/* ========================== Post Effects Render Target Implementation End ========================== */
}