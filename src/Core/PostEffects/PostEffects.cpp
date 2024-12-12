#include "PostEffects.h"

#include "../Graphics.h"
#include "../GraphicsDevice.h"
#include "../RenderPassManager.h"
#include "../UI.h"

#include "./GrayScale.h"

namespace PostEffects {
	bool GrayScale = true;
	bool Rendered = false;
}

void PostEffects::Initialize() {
	GrayScale::Initialize();
}

void PostEffects::Shutdown() {
	GrayScale::Shutdown();
}

void PostEffects::Render(const VkCommandBuffer& commandBuffer) {
	Rendered = false;

	if (GrayScale) {
		GrayScale::Render(commandBuffer, Graphics::g_PostEffectsRenderPass);
		Rendered = true;
	}
}

void PostEffects::RenderUI() {
	ImGui::SeparatorText("Post Effects");
	ImGui::Checkbox("GrayScale", &GrayScale);
}