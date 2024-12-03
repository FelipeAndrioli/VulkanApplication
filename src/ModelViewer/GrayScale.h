#pragma once

#include "../Core/GraphicsDevice.h"
#include "../Core/VulkanHeader.h"

namespace GrayScale {
	void Initialize();
	void Shutdown();
	void Render(const VkCommandBuffer& commandBuffer, const Graphics::RenderPass& renderPass);
}