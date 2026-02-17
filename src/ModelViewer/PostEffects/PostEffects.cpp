#include "PostEffects.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/UI.h"
#include "../Core/RenderTarget.h"

/*
	Notes:

	- Currently the Swap Chain uses the image format VK_FORMAT_B8G8R8A8_SRGB that enables the automatic gamma correction automatically at the end
	of the render pipeline. When the gamma correction effect is enabled from the shader it's actually apliying the correction a second time resulting
	in a over bright image. The effect was kept for demonstration purposes.
*/

namespace PostEffects {

	VkDescriptorSet						m_DescriptorSet			= VK_NULL_HANDLE;
	
	bool								Initialized				= false;
	bool								GrayScaleEnabled		= false;
	bool								GammaCorrectionEnabled	= false;
	uint32_t							m_Width					= 0;
	uint32_t							m_Height				= 0;
	PostEffectsGPUData					m_PostEffectsGPUData	= {};
	Graphics::Shader					m_QuadVertexShader		= {};
	Graphics::Shader					m_PostEffectsFragShader = {};
	Graphics::Buffer					m_UniformBuffer			= {};
	Graphics::PipelineState				m_PostEffectsPSO		= {};
	Graphics::PipelineStateDescription	m_PostEffectsPSODesc	= {};

	Graphics::InputLayout m_InputLayout = {
		.pushConstants = {},
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },	// resulting image from geometry rendering
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }			// UBO for post effects
		}
	};
}

void PostEffects::Initialize() {
	if (Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_UniformBuffer = gfxDevice->CreateBuffer(sizeof(PostEffectsGPUData));

#ifdef RUNTIME_SHADER_COMPILATION
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_QuadVertexShader,			"../src/Assets/Shaders/quad.vert");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_PostEffectsFragShader,	"../src/Assets/Shaders/post_effects.frag");
#else
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_QuadVertexShader,			"./Shaders/quad_vert.spv");
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_PostEffectsFragShader,	"./Shaders/post_effects_frag.spv");
#endif

	m_PostEffectsPSODesc.Name			= "Post Effects Pipeline";
	m_PostEffectsPSODesc.vertexShader	= &m_QuadVertexShader;
	m_PostEffectsPSODesc.fragmentShader = &m_PostEffectsFragShader;
	m_PostEffectsPSODesc.cullMode		= VK_CULL_MODE_NONE;
	m_PostEffectsPSODesc.noVertex		= true;
	m_PostEffectsPSODesc.psoInputLayout.push_back(PostEffects::m_InputLayout);

	Initialized = true;
}

void PostEffects::Shutdown() {
	if (!Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyPipeline(m_PostEffectsPSO);
	gfxDevice->DestroyShader(m_QuadVertexShader);
	gfxDevice->DestroyShader(m_PostEffectsFragShader);

	Initialized = false;
}

void PostEffects::Render(const VkCommandBuffer& commandBuffer, const Graphics::PostEffectsRenderTarget& renderTarget, const Graphics::GPUImage& colorBuffer) {
	if (!Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	if (m_PostEffectsPSO.pipeline == VK_NULL_HANDLE 
		|| colorBuffer.Description.Width != m_Width
		|| colorBuffer.Description.Height != m_Height) {

		m_Width		= colorBuffer.Description.Width;
		m_Height	= colorBuffer.Description.Height;

		if (m_PostEffectsPSO.pipeline != VK_NULL_HANDLE)
			gfxDevice->DestroyPipeline(m_PostEffectsPSO);

		gfxDevice->CreatePipelineState(m_PostEffectsPSODesc, m_PostEffectsPSO, renderTarget);
		gfxDevice->CreateDescriptorSet(m_PostEffectsPSO.descriptorSetLayout, m_DescriptorSet);
		gfxDevice->WriteDescriptor(m_InputLayout.bindings[0], m_DescriptorSet, colorBuffer);
		gfxDevice->WriteDescriptor(m_InputLayout.bindings[1], m_DescriptorSet, m_UniformBuffer);
	}

	m_PostEffectsGPUData.GrayScaleEnabled		= GrayScaleEnabled ? 1 : 0;
	m_PostEffectsGPUData.GammaCorrectionEnabled = GammaCorrectionEnabled ? 1 : 0;

	gfxDevice->UpdateBuffer(m_UniformBuffer, &m_PostEffectsGPUData);
	gfxDevice->BindDescriptorSet(m_DescriptorSet, commandBuffer, m_PostEffectsPSO.pipelineLayout, 0, 1);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostEffectsPSO.pipeline);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void PostEffects::RenderUI() {
	if (!Initialized)
		return;

	ImGui::SeparatorText("Post Effects");
	ImGui::Checkbox("Render Gray Scale", &GrayScaleEnabled);

	if (GrayScaleEnabled)
		ImGui::DragFloat("Gray Scale", &m_PostEffectsGPUData.GrayScale, 0.002f, 0.0f, 1.0f);

	ImGui::Checkbox("Render Gamma Correction", &GammaCorrectionEnabled);

	if (GammaCorrectionEnabled)
		ImGui::DragFloat("Gamma Correction", &m_PostEffectsGPUData.GammaCorrection, 0.002f, 0.0f, 3.0f);
}