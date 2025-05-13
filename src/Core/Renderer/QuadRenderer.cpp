#include "QuadRenderer.h"

#include "../RenderTarget.h"

QuadRenderer::QuadRenderer(const char* id, const char* vertexShaderPath, const char* fragShaderPath, uint32_t width, uint32_t height) {
	m_Id				= id;
	m_Width				= width;
	m_Height			= height;
	m_VertexShaderPath	= vertexShaderPath;
	m_FragShaderPath	= fragShaderPath;
}

QuadRenderer::~QuadRenderer() {
	CleanUp();
}

void QuadRenderer::StartUp() {
	m_RenderTarget = std::make_unique<Graphics::OffscreenRenderTarget>(m_Width, m_Height);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

#ifdef RUNTIME_SHADER_COMPILATION
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		m_VertexShaderPath);
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	m_FragShaderPath);
#else
	gfxDevice->LoadShader(VK_SHADER_STAGE_VERTEX_BIT,	m_VertexShader,		m_VertexShaderPath);
	gfxDevice->LoadShader(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader,	m_FragShaderPath);
#endif

	m_PSOInputLayout = {
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		}
	};

	if (m_PushConstant) {
		VkPushConstantRange pushConstant = { .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS, .offset = 0, .size = m_PushConstantSize };
		m_PSOInputLayout.pushConstants.push_back(pushConstant);
	}

	m_PSODesc.Name				= m_Id;
	m_PSODesc.vertexShader		= &m_VertexShader;
	m_PSODesc.fragmentShader	= &m_FragmentShader;
	m_PSODesc.cullMode			= VK_CULL_MODE_NONE;
	m_PSODesc.noVertex			= true;
	m_PSODesc.psoInputLayout	.push_back(m_PSOInputLayout);

	gfxDevice->CreatePipelineState(m_PSODesc, m_PSO, *m_RenderTarget);

	for (int i = 0; i < Graphics::FRAMES_IN_FLIGHT; i++)
		gfxDevice->CreateDescriptorSet(m_PSO.descriptorSetLayout, m_Set[i]);
}

void QuadRenderer::CleanUp() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyShader	(m_VertexShader);
	gfxDevice->DestroyShader	(m_FragmentShader);
	gfxDevice->DestroyPipeline	(m_PSO);

	m_RenderTarget.reset();
}

void QuadRenderer::Update(const float d, const float c, const InputSystem::Input& input) {

}

void QuadRenderer::RenderUI() {

}

void QuadRenderer::Render(const VkCommandBuffer& commandBuffer, const Graphics::GPUImage& image) {
	if (m_RenderTarget == nullptr)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_RenderTarget->Begin(commandBuffer);

	// do the descriptor set need to be written every frame with the image?
	gfxDevice->WriteDescriptor	(m_PSOInputLayout.bindings[0], m_Set[gfxDevice->GetCurrentFrameIndex()], image);
	gfxDevice->BindDescriptorSet(m_Set[gfxDevice->GetCurrentFrameIndex()], commandBuffer, m_PSO.pipelineLayout, 0, 1);

	if (m_PushConstant) {
		vkCmdPushConstants(commandBuffer, m_PSO.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, m_PushConstantSize, m_PushConstant);
	}

	vkCmdBindPipeline	(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PSO.pipeline);
	vkCmdDraw			(commandBuffer, 6, 1, 0, 0);

	m_RenderTarget->End(commandBuffer);
}

void QuadRenderer::SetPushConstant(size_t size, void* pushConstant) {
	m_PushConstantSize	= size;
	m_PushConstant		= pushConstant;
}

const Graphics::GPUImage& QuadRenderer::GetColorBuffer() {
	if (m_RenderTarget == nullptr)
		return {};

	return m_RenderTarget->GetColorBuffer();
}