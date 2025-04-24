#pragma once

#include "IRenderer.h"

#include <glm.hpp>
#include <memory>

#include "../../Assets/ShadowCamera.h"

#define MAX_MODELS 10

namespace Assets {
	class Model;
}

namespace Graphics {
	struct GPUImage;
	class DepthOnlyRenderTarget;
}

/*
	Point		Light - doesn't have direction
	Directional Light - has direction
	Spot		Light - has direction
*/

class ShadowRenderer : public IRenderer {

/*
TODO's: 
	[x] - Create a depth only render target
	[ ] - Re-setting the extent
	[ ] - Re-setting the precision
	[ ] - Better handling of model array
	[ ] - Directional Light Shadow
	[ ] - Spot Light Shadow
	[ ] - Point Light Shadow
	[ ] - Multiple Lights
	[ ] - Atlas optimization
	[ ] - Set initial/final image layouts to the render pass using the description struct

Known Issues:
	
	- Some double buffering is missing to the shadow image, when the light direction changes it's possible see many artifacts.
	- To use other precision formats other than 32 we must enable stencil to the buffer as well.
*/

public:
	ShadowRenderer() {};
	ShadowRenderer(uint32_t width, uint32_t height, uint32_t precision) : m_Width(width), m_Height(height), m_Precision(precision) {};

	void StartUp						()																override;
	void CleanUp						()																override;
	void Update							(const float d, const float c, const InputSystem::Input& input) override;
	void Render							(const VkCommandBuffer& commandBuffer)							override;
	void RenderUI						()																override;
	void SetExtent						(uint32_t width, uint32_t height);
	void SetPrecision					(uint32_t precision);

	void Render(const VkCommandBuffer& commandBuffer, const std::vector<std::shared_ptr<Assets::Model>>& models, const glm::mat4& lightViewProj);

	const Graphics::GPUImage& GetDepthBuffer();

private:
	void LoadResources();
private:

	struct ModelGPUData {
		glm::vec4 extra[12] = {};
		glm::mat4 Model		= glm::mat4(1.0f);
	};

	struct ShadowMappingGPUData {
		glm::vec4 extra[12] = {};
		glm::mat4 Light		= glm::mat4(1.0f);
	} m_ShadowMappingGPUData;

	struct PushConstant {
		int ModelIndex;
	};

	uint32_t							m_Width													= 800;
	uint32_t							m_Height												= 600;
	uint32_t							m_Precision												= 16;

	Graphics::InputLayout				m_PSOInputLayout										= {};
	Graphics::Buffer					m_ModelUBO[Graphics::FRAMES_IN_FLIGHT]					= {};
	Graphics::Buffer					m_ShadowMappingUBO[Graphics::FRAMES_IN_FLIGHT]			= {};
	Graphics::PipelineStateDescription	m_PSODesc												= {};
	Graphics::PipelineState				m_PSO													= {};
	Graphics::Shader					m_VertexShader											= {};
	Graphics::Shader					m_FragmentShader										= {};

	std::unique_ptr<Graphics::DepthOnlyRenderTarget> m_RenderTarget;
	
	std::array<ModelGPUData, MAX_MODELS> m_ModelGPUData;

	VkDescriptorSet m_Set = VK_NULL_HANDLE;
};

