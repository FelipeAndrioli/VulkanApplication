#pragma once

#include "../Core/VulkanHeader.h"

namespace Graphics {
	class IRenderTarget;
}

namespace GrayScale {
	void Initialize();
	void Shutdown();
	void Render(const VkCommandBuffer& commandBuffer, const Graphics::IRenderTarget& renderTarget);
}