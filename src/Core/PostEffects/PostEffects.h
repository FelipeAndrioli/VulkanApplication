#pragma once

#include "../VulkanHeader.h"

namespace PostEffects {
	
	extern bool GrayScale;
	extern bool Rendered;		// used to check if g_PostEffects buffer is filled.

	void Initialize();
	void Shutdown();
	void Render(const VkCommandBuffer& commandBuffer);
	void RenderUI();
}