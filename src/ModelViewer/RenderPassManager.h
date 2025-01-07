#pragma once

#include "../Core/VulkanHeader.h"
#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"

namespace Graphics {
	extern RenderPass g_ColorRenderPass;
	extern RenderPass g_PostEffectsRenderPass;
	extern RenderPass g_DebugRenderPass;

	void InitializeStaticRenderPasses(uint32_t width, uint32_t height);
	void ResizeRenderPasses(uint32_t width, uint32_t height);
	void ResizeRenderPass(RenderPass& renderPass, uint32_t width, uint32_t height);
	void ShutdownRenderPasses();
}