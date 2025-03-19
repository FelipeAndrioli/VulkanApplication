#pragma once

#include "VulkanHeader.h"
#include "Graphics.h"
#include "GraphicsDevice.h"

namespace Graphics {

	class IRenderTarget {
	public:
		virtual ~IRenderTarget			();
		virtual void Create				() = 0;
		virtual void Destroy			();
		virtual void Resize				(uint32_t width, uint32_t height);
		virtual void Begin				(const VkCommandBuffer& commandBuffer) = 0;
		virtual void End				(const VkCommandBuffer& commandBuffer) = 0;
		virtual VkFramebuffer& GetFramebuffer(int imageIndex);

		const VkExtent2D			GetExtent		() const { return { m_Width, m_Height }; }
		const Graphics::RenderPass& GetRenderPass	() const { return m_RenderPass; }
	protected:
		virtual void BeginRenderPass	(const VkCommandBuffer& commandBuffer);
		virtual void EndRenderPass		(const VkCommandBuffer& commandBuffer);
	protected:
		// support to color, depth, and resolve images 
		std::array<GPUImage, 3> m_Images;

		std::vector<VkFramebuffer> m_Framebuffers;
		std::vector<VkImageCopy> m_ImagesToCopy;

		Graphics::RenderPass m_RenderPass;
		
		uint32_t m_Width	= 0;
		uint32_t m_Height	= 0;
		
		int m_TotalImages	= 0;
		int m_ColorIndex	= -1;
		int m_ResolveIndex	= -1;
		int m_DepthIndex	= -1;

		bool m_Started = false;
	};

	class SwapChainRenderTarget : public IRenderTarget {
	public:
		SwapChainRenderTarget(uint32_t width, uint32_t height);
		void Create() override;

//		GPUImage& GetColorBuffer()	{ return m_Images[m_ColorIndex]; } // this should probably return the Swap Chain Image
		GPUImage& GetDepthBuffer()	{ return m_Images[m_DepthIndex]; }
		void CopyColor	(const GPUImage& colorBuffer, int positionX = 0, int positionY = 0);
		void Begin		(const VkCommandBuffer& commandBuffer)  override;
		void End		(const VkCommandBuffer& commandBuffer)  override;
		void Resize		(uint32_t width, uint32_t height)		override;
	private:
		VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	class OffscreenRenderTarget : public IRenderTarget {
	public:
		OffscreenRenderTarget			(uint32_t width, uint32_t height);
		void Create						()										override;
		void ChangeLayout				(VkImageLayout newLayout);
		void Begin						(const VkCommandBuffer& commandBuffer)  override;
		void End						(const VkCommandBuffer& commandBuffer)  override;
		const GPUImage& GetColorBuffer	()							const { return m_Images[m_ColorIndex]; }
		const GPUImage& GetDepthBuffer	()							const { return m_Images[m_DepthIndex]; }

	private:
		float m_PositionX = 0;
		float m_PositionY = 0;
	};

	class PostEffectsRenderTarget : public IRenderTarget {
	public:
		PostEffectsRenderTarget		(uint32_t width, uint32_t height);
		void Create					()										override;
		void Begin					(const VkCommandBuffer& commandBuffer)  override;
		void End					(const VkCommandBuffer& commandBuffer)  override;
		void ChangeLayout			(VkImageLayout newLayout);
		GPUImage& GetColorBuffer	()										{ return m_Images[m_ColorIndex]; }
	};
}