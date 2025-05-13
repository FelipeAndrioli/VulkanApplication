#pragma once

#include "IRenderer.h"

#include <memory>

namespace InputSystem {
	class Input;
}

namespace Graphics {
	class OffscreenRenderTarget;
	class GPUImage;
}

/*
	TODO's:
		[ ] - Create a color only render target
		[ ] - Implement resizing
*/
class QuadRenderer : public IRenderer {
public:

	QuadRenderer() {};
	QuadRenderer(const char* id, const char* vertexShaderPath, const char* fragShaderPath, uint32_t width, uint32_t height);

	QuadRenderer& operator=(const QuadRenderer& other) {
		this->m_Id					= other.m_Id;
		this->m_VertexShaderPath	= other.m_VertexShaderPath;
		this->m_FragShaderPath		= other.m_FragShaderPath;
		this->m_Width				= other.m_Width;
		this->m_Height				= other.m_Height;

		return *this;
	}

	~QuadRenderer();

	void StartUp		()																		override;
	void CleanUp		()																		override;
	void Update			(const float d, const float c, const InputSystem::Input& input)			override;
	void RenderUI		()																		override;
	void Render			(const VkCommandBuffer& commandBuffer, const Graphics::GPUImage& image);
	void Resize			(uint32_t width, uint32_t height);
	void SetPushConstant(size_t size, void* pushConstant);

	const Graphics::GPUImage& GetColorBuffer();

	const char* GetID	() { return m_Id;		}
	uint32_t GetWidth	() { return m_Width;	}
	uint32_t GetHeight	() { return m_Height;	}
private:
	void Render(const VkCommandBuffer& commandBuffer)									override {};
private:
	uint32_t							m_Width					= 800;
	uint32_t							m_Height				= 600;

	Graphics::InputLayout				m_PSOInputLayout		= {};
	Graphics::PipelineStateDescription	m_PSODesc				= {};
	Graphics::PipelineState				m_PSO					= {};
	Graphics::Shader					m_VertexShader			= {};
	Graphics::Shader					m_FragmentShader		= {};

	std::unique_ptr<Graphics::OffscreenRenderTarget> m_RenderTarget;
	
	VkDescriptorSet m_Set[Graphics::FRAMES_IN_FLIGHT] = {};

	void* m_PushConstant		= nullptr;
	uint32_t m_PushConstantSize	= 0;

	const char* m_VertexShaderPath;
	const char* m_FragShaderPath;
	const char* m_Id;
};