#pragma once

#include "IRenderer.h"

#include <glm.hpp>
#include <memory>

#include "../../Assets/ShadowCamera.h"
#include "../SceneComponents.h"

#define MAX_MODELS 10
#define MAX_LIGHT_SOURCES 2

namespace Assets {
	class Model;
}

namespace Graphics {
	struct GPUImage;
	class DepthOnlyRenderTarget;
}

class ShadowRenderer : public IRenderer {

/*
TODO's: 
	[x] - Create a depth only render target
	[ ] - Re-setting the extent
	[ ] - Re-setting the precision
	[ ] - Better handling of model array
	[x] - Directional Light Shadow
	[x] - Spot Light Shadow
	[ ] - Point Light Shadow
	[ ] - Multiple Lights
		[x] - Spot
		[x] - Directional
		[ ] - Point
	[ ] - Atlas optimization
	[ ] - Remove geometry shader

Known Issues:
	- Some double buffering is missing to the shadow image, when the light direction changes it's possible see many artifacts.
*/

public:
	ShadowRenderer() : m_Layers(1) {};
	ShadowRenderer(uint32_t maxLights) : m_Layers(maxLights > MAX_LIGHT_SOURCES ? MAX_LIGHT_SOURCES : maxLights) {};
	ShadowRenderer(uint32_t width, uint32_t height, uint32_t precision, uint32_t maxLights) 
		: m_Width(width), 
		m_Height(height), 
		m_Precision(precision), 
		m_Layers(maxLights > MAX_LIGHT_SOURCES ? MAX_LIGHT_SOURCES : maxLights) {};

	void StartUp		()																override;
	void CleanUp		()																override;
	void Update			(const float d, const float c, const InputSystem::Input& input) override;
	void Render			(const VkCommandBuffer& commandBuffer)							override;
	void RenderUI		()																override;
	void SetExtent		(uint32_t width, uint32_t height);
	void SetPrecision	(uint32_t precision);
	void Render			(const VkCommandBuffer& commandBuffer, const std::vector<std::shared_ptr<Assets::Model>>& models, const glm::mat4& lightViewProj);
	void Render			(const VkCommandBuffer& commandBuffer, const std::vector<std::shared_ptr<Assets::Model>>& models, const std::vector<Scene::LightComponent>& lights);

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
	};

	struct PushConstants {
		int ModelIndex;
		int ActiveLightSources;
	} m_PushConstants;

	uint32_t							m_Width											= 800;
	uint32_t							m_Height										= 600;
	uint32_t							m_Precision										= 16;
	uint32_t							m_Layers										= 0;

	Graphics::InputLayout				m_PSOInputLayout								= {};
	Graphics::Buffer					m_ModelUBO[Graphics::FRAMES_IN_FLIGHT]			= {};
	Graphics::Buffer					m_ShadowMappingUBO[Graphics::FRAMES_IN_FLIGHT]	= {};
	Graphics::PipelineStateDescription	m_PSODesc										= {};
	Graphics::PipelineState				m_PSO											= {};
	Graphics::Shader					m_VertexShader									= {};
	Graphics::Shader					m_GeometryShader								= {};	// for multilayer rendering
	Graphics::Shader					m_FragmentShader								= {};

	std::unique_ptr<Graphics::DepthOnlyRenderTarget> m_RenderTarget;
	
	std::array<ModelGPUData, MAX_MODELS> m_ModelGPUData;
	std::array<ShadowMappingGPUData, MAX_LIGHT_SOURCES> m_ShadowMappingGPUData;

	VkDescriptorSet m_Set = VK_NULL_HANDLE;
};

