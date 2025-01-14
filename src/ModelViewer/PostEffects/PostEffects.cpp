#include "PostEffects.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/RenderPassManager.h"
#include "../Core/UI.h"

#include "./GrayScale.h"

namespace PostEffects {
	bool GrayScale = false;
	bool Rendered = false;
}

void PostEffects::Initialize() {
	GrayScale::Initialize();
}

void PostEffects::Shutdown() {
	GrayScale::Shutdown();
}

void PostEffects::Render(const VkCommandBuffer& commandBuffer, const Graphics::GPUImage& sceneColor) {
	Rendered = false;

	if (GrayScale) {
		GrayScale::Render(commandBuffer, Graphics::g_PostEffectsRenderPass, sceneColor);
		Rendered = true;
	}
}

void PostEffects::RenderUI() {
	ImGui::SeparatorText("Post Effects");
	ImGui::Checkbox("GrayScale", &GrayScale);
}