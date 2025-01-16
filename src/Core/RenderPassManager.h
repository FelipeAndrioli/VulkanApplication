#pragma once

#include <unordered_map>
#include <vector>

#include "../Core/VulkanHeader.h"
#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"

namespace Graphics {
	extern RenderPass g_ColorRenderPass;
	extern RenderPass g_PostEffectsRenderPass;

	extern std::unordered_map<VkRenderPass, std::vector<VkFramebuffer>> framebufferMap;

	void InitializeStaticRenderPasses(uint32_t width, uint32_t height);
	void ResizeRenderPasses(uint32_t width, uint32_t height);
	void ResizeRenderPass(RenderPass& renderPass, uint32_t width, uint32_t height);
	void ShutdownRenderPasses();
}