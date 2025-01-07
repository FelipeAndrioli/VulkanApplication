#pragma once

#include "../Core/VulkanHeader.h"

namespace Graphics {
	struct GPUImage;
}

namespace PostEffects {
	
	extern bool GrayScale;
	extern bool Rendered;		// used to check if g_PostEffects buffer is filled.

	void Initialize();
	void Shutdown();
	void Render(const VkCommandBuffer& commandBuffer, const Graphics::GPUImage& sceneColor);
	void RenderUI();
}