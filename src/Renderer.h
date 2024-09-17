#pragma once

#include <memory>
#include <string>

#include "./Core/VulkanHeader.h"
#include "glm.hpp"

namespace Assets {
	class Camera;
	class Model;
}

namespace Graphics {
	struct PipelineState;
}

namespace Renderer {
	std::shared_ptr<Assets::Model> LoadModel(const std::string& path);

	void Init();
	void Shutdown();
	void LoadResources();
	void OnUIRender();

	void UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera);
	void RenderSkybox(const VkCommandBuffer& commandBuffer);
	void RenderModel(const VkCommandBuffer& commandBuffer, Assets::Model& model);
	void RenderWireframe(const VkCommandBuffer& commandBuffer, Assets::Model& model);
	void RenderLightSources(const VkCommandBuffer& commandBuffer);
	void RenderCube(const VkCommandBuffer& commandBuffer, const Graphics::PipelineState& PSO);
	void AddFlashLight(const Assets::Camera& camera);
}
