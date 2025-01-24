#pragma once

#include "../Core/VulkanHeader.h"

namespace Graphics {
	struct GPUImage;
	struct InputLayout;

	class PostEffectsRenderTarget;
}

namespace PostEffects {
	
	extern bool GrayScale;
	extern bool Rendered;		
	extern bool Initialized;

	extern Graphics::InputLayout m_InputLayout;

	void Initialize();
	void Shutdown();
	void Render(
		const VkCommandBuffer& commandBuffer, 
		const Graphics::PostEffectsRenderTarget& renderTarget, 
		const Graphics::GPUImage& colorBuffer);
	void RenderUI();
}