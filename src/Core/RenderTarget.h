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

		const VkExtent2D GetExtent							() const { return { m_Width, m_Height }; }
		virtual const VkViewport GetViewport				() const { return m_RenderPass.Description.Viewport; }
		virtual const VkRect2D GetScissor					() const { return m_RenderPass.Description.Scissor; }
		virtual const VkSampleCountFlagBits GetSampleCount	() const { return m_RenderPass.Description.SampleCount; }
		virtual const Graphics::RenderPass& GetRenderPass	() const { return m_RenderPass; }
		virtual const VkRenderPass& GetRenderPassHandle		() const { return m_RenderPass.Handle; }
		virtual const uint32_t GetColorAttachmentCount		() const { return 1; }
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

		// Note: When specifying a image format different than the swap chain format it must be converted first before being copied/blitted to the swap chain (HDR -> LDR).
		OffscreenRenderTarget			(uint32_t width, uint32_t height, VkFormat imageFormat);

		OffscreenRenderTarget			(uint32_t width, uint32_t height, VkFormat imageFormat, uint32_t numColorAttachments);
		void Create						()										override;
		void ChangeLayout				(VkImageLayout newLayout);
		void Begin						(const VkCommandBuffer& commandBuffer)  override;
		void End						(const VkCommandBuffer& commandBuffer)  override;
		const GPUImage& GetColorBuffer	()							const { return m_Images[m_ColorIndex]; }
		const GPUImage& GetDepthBuffer	()							const { return m_Images[m_DepthIndex]; }

	private:
		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;

		uint32_t m_NumColorAttachments = 0;

		float m_PositionX = 0;
		float m_PositionY = 0;
	};

	class PostEffectsRenderTarget : public IRenderTarget {
	public:
		PostEffectsRenderTarget		(uint32_t width, uint32_t height);
		PostEffectsRenderTarget(uint32_t width, uint32_t height, Graphics::Format imageFormat, Graphics::ResourceState finalResourceState);
		void Create					()										override;
		void Begin					(const VkCommandBuffer& commandBuffer)  override;
		void End					(const VkCommandBuffer& commandBuffer)  override;
		void ChangeLayout			(VkImageLayout newLayout);
		GPUImage& GetColorBuffer	()										{ return m_Images[m_ColorIndex]; }

	private:
		Graphics::Format m_ImageFormat = Graphics::Format::UNKNOWN;
		Graphics::ResourceState m_FinalResourceState = Graphics::ResourceState::UNDEFINED;
	};

	class DepthOnlyRenderTarget : public IRenderTarget {
	public:
		DepthOnlyRenderTarget			(uint32_t width, uint32_t height, uint32_t precision, uint32_t layers);
		void Create						()														override;
		void Begin						(const VkCommandBuffer& commandBuffer)					override;
		void End						(const VkCommandBuffer& commandBuffer)					override;
		const GPUImage& GetDepthBuffer	()														{ return m_Images[m_DepthIndex]; }
	private:
		uint32_t m_Precision	= 0;
		uint32_t m_Layers		= 0;
	};

	constexpr uint32_t CUBE_FACES = 6;

	class DepthOnlyCubeRenderTarget : public IRenderTarget {
	public:
		DepthOnlyCubeRenderTarget(uint32_t width, uint32_t height, uint32_t precision, uint32_t layers, bool singleFramebuffer);
		~DepthOnlyCubeRenderTarget();
		void Begin(const VkCommandBuffer& commandBuffer, uint32_t layer = 0);
		void Create()										override;
		void End(const VkCommandBuffer& commandBuffer)		override;
		void Resize(uint32_t width, uint32_t height)		override;
		const GPUImage& GetDepthBuffer() { return m_DepthCubeMap; }
	private:
		void CreateSingleFramebuffer();
		void CreateMultipleFramebuffer();

		void BeginSingleFramebuffer(const VkCommandBuffer& commandBuffer);
		void BeginMultipleFramebuffer(const VkCommandBuffer& commandBuffer, uint32_t layer);

		void Begin(const VkCommandBuffer& commandBuffer) override;
	private:

		GPUImageCube m_DepthCubeMap;

		uint32_t m_Precision;
		uint32_t m_Layers;
		
		bool m_SingleFramebuffer = false;
	};

	// Note:	I'll go with specific 'render targets' for now and will change the way they're being created when
	//			refactoring everything.
	class MultiAttachmentRenderTarget : public IRenderTarget {
	public:

		struct AttachmentDescription {
			uint32_t Width;
			uint32_t Height;
			uint32_t Samples;
			Graphics::Format ImageFormat;
		};

		MultiAttachmentRenderTarget(const uint32_t width, const uint32_t height, const uint32_t numColorAttachments, const Graphics::Format imageFormat);
		MultiAttachmentRenderTarget(const uint32_t width, const uint32_t height, const std::vector<AttachmentDescription> colorAttachmentDescriptions);
		~MultiAttachmentRenderTarget();

		void Begin			(const VkCommandBuffer& commandBuffer)	override;
		void Create			()										override;
		void End			(const VkCommandBuffer& commandBuffer)	override;
		void Resize			(uint32_t width, uint32_t height)		override;
		void ChangeLayout	(VkImageLayout newLayout);
		
//		const GPUImage&				GetColorBuffer			()	{ return m_ResolveAttachments[1]; }
		const std::vector<Graphics::GPUImage>& GetColorBuffer			()	{ return m_ColorAttachments; }
		const std::vector<Graphics::GPUImage>& GetResolvedColorBuffer	()	{ return m_ResolveAttachments; }
		const VkSampleCountFlagBits GetSampleCount			()	const override;
		const VkRenderPass&			GetRenderPassHandle		()	const override;
		const VkViewport			GetViewport				() const override;
		const VkRect2D				GetScissor				() const override;
		const uint32_t				GetColorAttachmentCount () const override;
	private:
		uint32_t m_NumColorAttachments = 0;

		Graphics::Format m_ImageFormat = Graphics::Format::UNKNOWN;

		std::vector<Graphics::GPUImage> m_ColorAttachments;
		std::vector<Graphics::GPUImage> m_DepthAttachments;
		std::vector<Graphics::GPUImage> m_ResolveAttachments;

		std::vector<VkClearValue> m_ClearValues;
		std::vector<VkImageView> m_FramebufferViews;

		Graphics::RenderPassDescription m_RenderPassDescription = {};
	};
}