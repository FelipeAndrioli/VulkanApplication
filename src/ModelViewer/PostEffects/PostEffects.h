#pragma once

#include <glm/glm.hpp>

#include "../Core/VulkanHeader.h"

namespace Graphics {
	struct GPUImage;
	struct InputLayout;

	class PostEffectsRenderTarget;
}

namespace PostEffects {
	struct PostEffectsGPUData {
		float		GrayScale				= 0.5f;
		float		GammaCorrection			= 2.2f;
		int			GrayScaleEnabled		= 0;		// 0 - disabled, 1 - enabled
		int			GammaCorrectionEnabled  = 0;
		glm::vec4	extra[15]				= {};
	};

	extern bool	Initialized;

	void Initialize();
	void Shutdown();
	void Render(const VkCommandBuffer& commandBuffer, const Graphics::PostEffectsRenderTarget& renderTarget, const Graphics::GPUImage& colorBuffer);
	void RenderUI();
}