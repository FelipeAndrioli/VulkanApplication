#pragma once

#include <memory>
#include <string>

#include "VulkanHeader.h"
#include "glm.hpp"

namespace Assets {
	class Camera;
	class Model;
}

namespace Renderer {
	std::shared_ptr<Assets::Model> LoadModel(const std::string& path);

	void Init();
	void LoadResources();
	void Destroy();

	void UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera);
	void RenderSkybox(const VkCommandBuffer& commandBuffer);
	void RenderModel(const VkCommandBuffer& commandBuffer, Assets::Model& model);
	void RenderWireframe(const VkCommandBuffer& commandBuffer, Assets::Model& model);
}