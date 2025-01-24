#include "PostEffects.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/UI.h"
#include "../Core/RenderTarget.h"

#include "./GrayScale.h"

namespace PostEffects {
	bool GrayScale = false;
	bool Rendered = false;
	bool Initialized = false;

	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_DescriptorSet[Graphics::FRAMES_IN_FLIGHT];
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	Graphics::InputLayout m_InputLayout = {
		.pushConstants = {},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		}
	};
}

void PostEffects::Initialize() {
	if (Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	gfxDevice->CreateDescriptorSetLayout(m_DescriptorSetLayout, m_InputLayout.bindings);
	gfxDevice->CreatePipelineLayout(m_DescriptorSetLayout, m_PipelineLayout);
	
	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++) {
		gfxDevice->CreateDescriptorSet(m_DescriptorSetLayout, m_DescriptorSet[i]);
	}

	GrayScale::Initialize();

	Initialized = true;
}

void PostEffects::Shutdown() {
	if (!Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	gfxDevice->DestroyDescriptorSetLayout(m_DescriptorSetLayout);
	gfxDevice->DestroyPipelineLayout(m_PipelineLayout);

	GrayScale::Shutdown();

	Initialized = false;
}

void PostEffects::Render(
	const VkCommandBuffer& commandBuffer, 
	const Graphics::PostEffectsRenderTarget& renderTarget, 
	const Graphics::GPUImage& colorBuffer) {
 
	if (!Initialized)
		return;

	Rendered = false;
	
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->WriteDescriptor(
		m_InputLayout.bindings[0],
		m_DescriptorSet[gfxDevice->GetCurrentFrameIndex()],
		colorBuffer
		);

	gfxDevice->BindDescriptorSet(
		m_DescriptorSet[gfxDevice->GetCurrentFrameIndex()], 
		commandBuffer,
		m_PipelineLayout, 0, 1
	);

	if (GrayScale) {
		GrayScale::Render(commandBuffer, renderTarget);
		Rendered = true;
	}
}

void PostEffects::RenderUI() {
	if (!Initialized)
		return;

	ImGui::SeparatorText("Post Effects");
	ImGui::Checkbox("GrayScale", &GrayScale);
}