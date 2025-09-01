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

		m_RenderPass.Description.Viewport.width = width;
		m_RenderPass.Description.Viewport.height = height;
		m_RenderPass.Description.Extent = GetExtent();
		m_RenderPass.Description.Scissor.extent = GetExtent();

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->DestroyFramebuffer(m_Framebuffers);

		std::vector<VkImageView> attachments = {};

		if (m_ResolveIndex != -1) {
			gfxDevice->ResizeImage(m_Images[m_ResolveIndex], width, height);
			gfxDevice->CreateImageSampler(m_Images[m_ResolveIndex]);

			attachments.emplace_back(m_Images[m_ResolveIndex].ImageView);
		}

		if (m_DepthIndex != -1) {
			gfxDevice->ResizeImage(m_Images[m_DepthIndex], width, height);
			
			attachments.emplace_back(m_Images[m_DepthIndex].ImageView);
		}

		if (m_ColorIndex != -1) {
			gfxDevice->ResizeImage(m_Images[m_ColorIndex], width, height);
			gfxDevice->CreateImageSampler(m_Images[m_ColorIndex]);

			gfxDevice->TransitionImageLayout(m_Images[m_ColorIndex], VK_IMAGE_LAYOUT_UNDEFINED, m_RenderPass.FinalLayout);
		
			attachments.emplace_back(m_Images[m_ColorIndex].ImageView);
		}

		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		for (int i = 0; i < m_Framebuffers.size(); i++) {
			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, attachments, GetExtent(), m_Framebuffers[i]);
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
		renderPassBeginInfo.renderArea.offset = m_RenderPass.Description.Offset;
		renderPassBeginInfo.renderArea.extent = m_RenderPass.Description.Extent;
		renderPassBeginInfo.pNext = nullptr;

		if (m_RenderPass.Description.ClearValues.size() == 0) {
			std::array<VkClearValue, 3> clearValues = {};
			int clearValuesCount = 0;

			if (m_RenderPass.Description.Flags & eColorAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };
			if (m_RenderPass.Description.Flags & eDepthAttachment)
				clearValues[clearValuesCount++] = { .depthStencil{ 1.0f, 0 } };
			if (m_RenderPass.Description.Flags & eResolveAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };

			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValuesCount);
			renderPassBeginInfo.pClearValues = clearValues.data();
		}
		else {
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(m_RenderPass.Description.ClearValues.size());
			renderPassBeginInfo.pClearValues = m_RenderPass.Description.ClearValues.data();
		}

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = m_RenderPass.Description.Viewport;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = m_RenderPass.Description.Scissor;
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
		m_Width		= width;
		m_Height	= height;

		Create();
	}

	void SwapChainRenderTarget::Create() {

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
		
		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		// Create swap chain render pass
		m_RenderPass.Description.Extent = GetExtent();
		m_RenderPass.Description.Viewport = {
			.x = 0,
			.y = 0,
			.width = static_cast<float>(m_Width),
			.height = static_cast<float>(m_Height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		m_RenderPass.Description.Scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = m_Width,
				.height = m_Height
			}
		};
		m_RenderPass.Description.SampleCount = VK_SAMPLE_COUNT_1_BIT;
		m_RenderPass.Description.Flags = eColorAttachment
			| eDepthAttachment
			| eColorLoadOpLoad
			| eColorStoreOpStore
			| eInitialLayoutColorOptimal
			| eFinalLayoutPresent;
		m_RenderPass.Description.ColorImageFormat = gfxDevice->GetSwapChain().ImageFormat;
		m_RenderPass.Description.DepthImageFormat = gfxDevice->GetDepthFormat();

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
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
		if (m_ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
			gfxDevice->TransitionImageLayout(
				gfxDevice->GetSwapChain().Images[gfxDevice->GetSwapChain().ImageIndex],
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				0,
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			);
		}
		
		m_ImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		BeginRenderPass(commandBuffer);
	}
	
	void SwapChainRenderTarget::End(const VkCommandBuffer& commandBuffer) {
		EndRenderPass(commandBuffer);
//		m_ImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	void SwapChainRenderTarget::Resize(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		m_RenderPass.Description.Viewport.width = width;
		m_RenderPass.Description.Viewport.height = height;
		m_RenderPass.Description.Extent = GetExtent();
		m_RenderPass.Description.Scissor.extent = GetExtent();

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->ResizeImage(m_Images[m_DepthIndex], width, height);

		gfxDevice->DestroyFramebuffer(m_Framebuffers);

		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		for (int i = 0; i < m_Framebuffers.size(); i++) {
			std::vector<VkImageView> attachments = {
				gfxDevice->GetSwapChain().ImageViews[i],
				m_Images[m_DepthIndex].ImageView
			};

			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, attachments, GetExtent(), m_Framebuffers[i]);
		}

	}

	/* ========================== SwapChain Render Target Implementation End ========================== */

	/* ========================== Offscreen Render Target Implementation Begin ========================== */

	OffscreenRenderTarget::OffscreenRenderTarget(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
		m_ImageFormat = gfxDevice->GetSwapChain().ImageFormat;
		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		Create();
	}

	OffscreenRenderTarget::OffscreenRenderTarget(uint32_t width, uint32_t height, VkFormat imageFormat) {
		m_Width = width;
		m_Height = height;

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
		m_ImageFormat = imageFormat;
		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		Create();
	}

	void OffscreenRenderTarget::Create() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_RenderPass.Description.Extent = GetExtent();
		m_RenderPass.Description.Viewport = {
			.x = 0, 
			.y = 0,
			.width = static_cast<float>(m_Width),
			.height = static_cast<float>(m_Height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		m_RenderPass.Description.Scissor = {
			.offset = {
				.x = 0, 
				.y = 0 
			},
			.extent = {
				.width = m_Width,
				.height = m_Height
			}
		};
		m_RenderPass.Description.SampleCount = gfxDevice->m_MsaaSamples;
		m_RenderPass.Description.Flags = eColorAttachment | eDepthAttachment | eColorLoadOpClear | eColorStoreOpStore;
		m_RenderPass.Description.ColorImageFormat = m_ImageFormat;
		m_RenderPass.Description.DepthImageFormat = gfxDevice->GetDepthFormat();

		if (!(gfxDevice->m_MsaaSamples & VK_SAMPLE_COUNT_1_BIT))
			m_RenderPass.Description.Flags |= eResolveAttachment;

		gfxDevice->CreateRenderPass(m_RenderPass);

		if (m_RenderPass.Description.Flags & eResolveAttachment) {
			m_ResolveIndex = m_TotalImages++;

			gfxDevice->CreateRenderTarget(
				m_Images[m_ResolveIndex],
				m_ImageFormat,
				GetExtent(),
				gfxDevice->m_MsaaSamples);
			gfxDevice->TransitionImageLayout(m_Images[m_ResolveIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			gfxDevice->CreateImageSampler(m_Images[m_ResolveIndex]);
		}

		m_DepthIndex = m_TotalImages++;
		gfxDevice->CreateDepthBuffer(m_Images[m_DepthIndex], GetExtent(), gfxDevice->m_MsaaSamples);

		m_ColorIndex = m_TotalImages++;
		gfxDevice->CreateRenderTarget(
			m_Images[m_ColorIndex],
			m_ImageFormat,
			GetExtent(),
			VK_SAMPLE_COUNT_1_BIT);
		gfxDevice->TransitionImageLayout(m_Images[m_ColorIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		gfxDevice->CreateImageSampler(m_Images[m_ColorIndex]);

		std::vector<VkImageView> views = {};

		if (m_RenderPass.Description.Flags & eResolveAttachment) {
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

		if (m_Images[m_ColorIndex].ImageLayout != newLayout)
			gfxDevice->TransitionImageLayout(m_Images[m_ColorIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, newLayout);
	}

	void OffscreenRenderTarget::Begin(const VkCommandBuffer& commandBuffer) {
		BeginRenderPass(commandBuffer);
	}
	
	void OffscreenRenderTarget::End(const VkCommandBuffer& commandBuffer) {
		EndRenderPass(commandBuffer);

		m_Images[m_ColorIndex].ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (m_RenderPass.Description.Flags & eResolveAttachment)
			m_Images[m_ResolveIndex].ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

		m_RenderPass.Description.Extent = GetExtent();
		m_RenderPass.Description.Viewport = {
			.x = 0,
			.y = 0,
			.width = static_cast<float>(m_Width),
			.height = static_cast<float>(m_Height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		m_RenderPass.Description.Scissor = {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width = m_Width,
				.height = m_Height
			}
		};

		m_RenderPass.Description.SampleCount = VK_SAMPLE_COUNT_1_BIT;
		m_RenderPass.Description.Flags = eColorAttachment | eColorLoadOpClear | eColorStoreOpStore | eFinalLayoutTransferSrc;
		m_RenderPass.Description.ColorImageFormat = gfxDevice->GetSwapChain().ImageFormat;

		gfxDevice->CreateRenderPass(m_RenderPass);

		m_ColorIndex = m_TotalImages++;

		gfxDevice->CreateRenderTarget(m_Images[m_ColorIndex], gfxDevice->GetSwapChain().ImageFormat, GetExtent(), VK_SAMPLE_COUNT_1_BIT);
		gfxDevice->TransitionImageLayout(m_Images[m_ColorIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		gfxDevice->CreateImageSampler(m_Images[m_ColorIndex]);

		std::vector<VkImageView> views = { m_Images[m_ColorIndex].ImageView };

		for (int i = 0; i < m_Framebuffers.size(); i++) {
			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, views, GetExtent(), m_Framebuffers[i]);
		}
	}

	void PostEffectsRenderTarget::ChangeLayout(VkImageLayout newLayout) {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		if (m_Images[m_ColorIndex].ImageLayout != newLayout)
			gfxDevice->TransitionImageLayout(m_Images[m_ColorIndex], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, newLayout);
	}

	void PostEffectsRenderTarget::Begin(const VkCommandBuffer& commandBuffer) {
		BeginRenderPass(commandBuffer);
	}
	
	void PostEffectsRenderTarget::End(const VkCommandBuffer& commandBuffer) {
		EndRenderPass(commandBuffer);
	}

	/* ========================== Post Effects Render Target Implementation End ========================== */

	/*  ========================== Depth Only Render Target Implementation Begin ========================== */

	DepthOnlyRenderTarget::DepthOnlyRenderTarget(uint32_t width, uint32_t height, uint32_t precision, uint32_t layers) {
		m_Width		= width;
		m_Height	= height;
		m_Precision = precision;
		m_Layers	= layers;

		Create();
	}

	void DepthOnlyRenderTarget::Create() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		m_RenderPass.Description.Extent		= GetExtent();
		m_RenderPass.Description.Viewport	= {
			.x			= 0,
			.y			= 0,
			.width		= static_cast<float>(m_Width),
			.height		= static_cast<float>(m_Height),
			.minDepth	= 0.0f,
			.maxDepth	= 1.0f
		};
		m_RenderPass.Description.Scissor	= {
			.offset = {
				.x = 0,
				.y = 0
			},
			.extent = {
				.width	= m_Width,
				.height = m_Height
			}
		};

		m_RenderPass.Description.SampleCount		= VK_SAMPLE_COUNT_1_BIT;
		m_RenderPass.Description.Flags				= eDepthAttachment | eColorLoadOpClear | eColorStoreOpStore;
		m_RenderPass.Description.DepthImageFormat	= gfxDevice->GetDepthOnlyFormat();

		gfxDevice->CreateRenderPass(m_RenderPass);

		m_DepthIndex = m_TotalImages++;

		gfxDevice->CreateDepthOnlyBuffer(m_Images[m_DepthIndex], GetExtent(), VK_SAMPLE_COUNT_1_BIT, m_Layers);
		// transition to depth/stencil attachment optimal? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		gfxDevice->CreateImageSampler	(m_Images[m_DepthIndex]);

		std::vector<VkImageView> views = { m_Images[m_DepthIndex].ImageView };

		for (int i = 0; i < m_Framebuffers.size(); i++) {
			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, views, GetExtent(), m_Framebuffers[i], m_Layers);
		}
	}

	void DepthOnlyRenderTarget::Begin(const VkCommandBuffer& commandBuffer) {
		BeginRenderPass(commandBuffer);
	}

	void DepthOnlyRenderTarget::End(const VkCommandBuffer& commandBuffer) {
		EndRenderPass(commandBuffer);
		m_Images[m_DepthIndex].ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}

	/*  ========================== Depth Only Render Target Implementation End ========================== */

	/*  ========================== Depth Only Cube Render Target Implementation Begin ========================== */

	DepthOnlyCubeRenderTarget::DepthOnlyCubeRenderTarget(uint32_t width, uint32_t height, uint32_t precision, uint32_t layers, bool singleFramebuffer) {

		m_Width				= width > height ? width : height;
		m_Height			= m_Width;
		m_Precision			= precision;
		m_Layers			= layers;
		m_SingleFramebuffer = singleFramebuffer;

		Create();
	}

	DepthOnlyCubeRenderTarget::~DepthOnlyCubeRenderTarget() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->DestroyImageCube(m_DepthCubeMap);
	}

	void DepthOnlyCubeRenderTarget::CreateSingleFramebuffer() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		if (m_RenderPass.Handle == VK_NULL_HANDLE) {
			m_RenderPass.Description.Extent		= GetExtent();
			m_RenderPass.Description.Viewport	= {
				.x			= 0,
				.y			= 0,
				.width		= static_cast<float>(m_Width),
				.height		= static_cast<float>(m_Height),
				.minDepth	= 0.0f,
				.maxDepth	= 1.0f
			};

			m_RenderPass.Description.Scissor	= {
				.offset = {
					.x = 0,
					.y = 0
				},
				.extent = {
					.width	= m_Width,
					.height = m_Height
				}
			};

			m_RenderPass.Description.SampleCount		= VK_SAMPLE_COUNT_1_BIT;
			m_RenderPass.Description.Flags				= eDepthAttachment | eColorLoadOpClear | eColorStoreOpStore;
			m_RenderPass.Description.DepthImageFormat	= gfxDevice->GetDepthOnlyFormat();

			gfxDevice->CreateRenderPass(m_RenderPass);
		}

		m_Framebuffers.resize(gfxDevice->GetSwapChain().ImageViews.size());

		ImageDescription desc	= {};
		desc.Width				= m_Width;
		desc.Height				= m_Height;
		desc.MipLevels			= 1;
		desc.MsaaSamples		= VK_SAMPLE_COUNT_1_BIT;
		desc.Tiling				= VK_IMAGE_TILING_OPTIMAL;
		desc.Usage				= static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		desc.MemoryProperty		= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		desc.AspectFlags		= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		desc.AspectMask			= VK_IMAGE_ASPECT_DEPTH_BIT;
		desc.ViewType			= VK_IMAGE_VIEW_TYPE_CUBE;
		desc.LayerCount			= CUBE_FACES;
		desc.AddressMode		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		desc.Format				= VK_FORMAT_D32_SFLOAT;
		desc.ImageType			= VK_IMAGE_TYPE_2D;
		desc.SamplerCompareOp	= VK_COMPARE_OP_NEVER;

		gfxDevice->CreateImage(m_DepthCubeMap, desc);
		gfxDevice->CreateImageView(m_DepthCubeMap.Image, m_DepthCubeMap.ImageView, m_DepthCubeMap.Description);
		gfxDevice->CreateImageSampler(m_DepthCubeMap);

		for (uint32_t i = 0; i < m_Framebuffers.size(); i++) {
			std::vector<VkImageView> views = { m_DepthCubeMap.ImageView };

			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, views, GetExtent(), m_Framebuffers[i], CUBE_FACES);
		}

		gfxDevice->TransitionCubeImageLayout(m_DepthCubeMap, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	}
	
	void DepthOnlyCubeRenderTarget::CreateMultipleFramebuffer() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		if (m_RenderPass.Handle == VK_NULL_HANDLE) {
			m_RenderPass.Description.Extent		= GetExtent();
			m_RenderPass.Description.Viewport	= {
				.x			= 0,
				.y			= 0,
				.width		= static_cast<float>(m_Width),
				.height		= static_cast<float>(m_Height),
				.minDepth	= 0.0f,
				.maxDepth	= 1.0f
			};

			m_RenderPass.Description.Scissor	= {
				.offset = {
					.x = 0,
					.y = 0
				},
				.extent = {
					.width	= m_Width,
					.height = m_Height
				}
			};

			m_RenderPass.Description.SampleCount		= VK_SAMPLE_COUNT_1_BIT;
			m_RenderPass.Description.Flags				= eDepthAttachment | eColorLoadOpClear | eColorStoreOpStore;
			m_RenderPass.Description.DepthImageFormat	= gfxDevice->GetDepthOnlyFormat();

			gfxDevice->CreateRenderPass(m_RenderPass);
		}

		m_Framebuffers.resize(CUBE_FACES);

		ImageDescription desc	= {};
		desc.Width				= m_Width;
		desc.Height				= m_Height;
		desc.MipLevels			= 1;
		desc.MsaaSamples		= VK_SAMPLE_COUNT_1_BIT;
		desc.Tiling				= VK_IMAGE_TILING_OPTIMAL;
		desc.Usage				= static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		desc.MemoryProperty		= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		desc.AspectFlags		= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		desc.AspectMask			= VK_IMAGE_ASPECT_DEPTH_BIT;
		desc.ViewType			= VK_IMAGE_VIEW_TYPE_CUBE;
		desc.LayerCount			= CUBE_FACES;
		desc.AddressMode		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		desc.Format				= VK_FORMAT_D32_SFLOAT;
		desc.ImageType			= VK_IMAGE_TYPE_2D;
		desc.SamplerCompareOp	= VK_COMPARE_OP_NEVER;

		gfxDevice->CreateImage(m_DepthCubeMap, desc);
		gfxDevice->CreateImageView(m_DepthCubeMap.Image, m_DepthCubeMap.ImageView, m_DepthCubeMap.Description);

		m_DepthCubeMap.Description.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		m_DepthCubeMap.Description.LayerCount = 1;

		for (uint32_t i = 0; i < CUBE_FACES; i++) {
			m_DepthCubeMap.Description.BaseArrayLayer = i;
	
			gfxDevice->CreateImageView(m_DepthCubeMap.Image, m_DepthCubeMap.ImageViews[i], m_DepthCubeMap.Description);
		}

		gfxDevice->CreateImageSampler(m_DepthCubeMap);

		// Note: Cube Framebuffers are capped to one per cube face as of now, it will hold performance and more should be added later. 
		for (uint32_t i = 0; i < m_Framebuffers.size(); i++) {
			std::vector<VkImageView> views = { m_DepthCubeMap.ImageViews[i]};

			gfxDevice->CreateFramebuffer(m_RenderPass.Handle, views, GetExtent(), m_Framebuffers[i], 1);
		}

		gfxDevice->TransitionCubeImageLayout(m_DepthCubeMap, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
	}

	void DepthOnlyCubeRenderTarget::Create() {
		if (m_SingleFramebuffer) {
			CreateSingleFramebuffer();
		}
		else {
			CreateMultipleFramebuffer();
		}
	}

	void DepthOnlyCubeRenderTarget::BeginSingleFramebuffer(const VkCommandBuffer& commandBuffer) {
		BeginRenderPass(commandBuffer);
	}

	void DepthOnlyCubeRenderTarget::BeginMultipleFramebuffer(const VkCommandBuffer& commandBuffer, uint32_t layer) {
		if (m_Started)
			return;

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		VkRenderPassBeginInfo renderPassBeginInfo	= {};
		renderPassBeginInfo.sType					= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass				= m_RenderPass.Handle;
		renderPassBeginInfo.framebuffer				= m_Framebuffers[layer];
		renderPassBeginInfo.renderArea.offset		= m_RenderPass.Description.Offset;
		renderPassBeginInfo.renderArea.extent		= m_RenderPass.Description.Extent;
		renderPassBeginInfo.pNext					= nullptr;

		if (m_RenderPass.Description.ClearValues.size() == 0) {
			std::array<VkClearValue, 3> clearValues = {};
			int clearValuesCount = 0;

			if (m_RenderPass.Description.Flags & eColorAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };
			if (m_RenderPass.Description.Flags & eDepthAttachment)
				clearValues[clearValuesCount++] = { .depthStencil{ 1.0f, 0 } };
			if (m_RenderPass.Description.Flags & eResolveAttachment)
				clearValues[clearValuesCount++] = { .color{0.0f, 0.0f, 0.0f, 1.0f} };

			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValuesCount);
			renderPassBeginInfo.pClearValues	= clearValues.data();
		}
		else {
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(m_RenderPass.Description.ClearValues.size());
			renderPassBeginInfo.pClearValues	= m_RenderPass.Description.ClearValues.data();
		}

		VkViewport viewport = m_RenderPass.Description.Viewport;
		VkRect2D scissor	= m_RenderPass.Description.Scissor;
		
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		m_Started = true;
	}

	void DepthOnlyCubeRenderTarget::Begin(const VkCommandBuffer& commandBuffer, uint32_t layer) {
		if (m_SingleFramebuffer) {
			BeginSingleFramebuffer(commandBuffer);
		}
		else {
			BeginMultipleFramebuffer(commandBuffer, layer);
		}
	}

	void DepthOnlyCubeRenderTarget::End(const VkCommandBuffer& commandBuffer) {
		EndRenderPass(commandBuffer);
		m_DepthCubeMap.ImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}

	void DepthOnlyCubeRenderTarget::Resize(uint32_t width, uint32_t height) {
		m_Width = width > height ? width : height;
		m_Height = m_Width;

		m_RenderPass.Description.Viewport.width = m_Width;
		m_RenderPass.Description.Viewport.height = m_Height;
		m_RenderPass.Description.Extent = GetExtent();
		m_RenderPass.Description.Scissor.extent = GetExtent();
		
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->WaitIdle();

		gfxDevice->DestroyFramebuffer(m_Framebuffers);
		gfxDevice->DestroyImageCube(m_DepthCubeMap);
	
		Create();
	}

	void DepthOnlyCubeRenderTarget::Begin(const VkCommandBuffer& commandBuffer) {

	}

	/*  ========================== Depth Only Cube Render Target Implementation End ========================== */
}