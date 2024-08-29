#pragma once

#include "VulkanHeader.h"
#include "Assets/Camera.h"
#include "Assets/Model.h"

namespace Renderer {
	std::shared_ptr<Model> LoadModel(const std::string& path);

	void Init();
	void LoadResources();
	void Destroy();

	void UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera);
	void RenderSkybox(const VkCommandBuffer& commandBuffer);
	void RenderModel(const VkCommandBuffer& commandBuffer, Model& model);
	void RenderWireframe(const VkCommandBuffer& commandBuffer, Model& model);

	struct SceneGPUData {
		float time = 0.0f;
		float extra_s_1 = 0.0f;
		float extra_s_2 = 0.0f;
		float extra_s_3 = 0.0f;
		glm::vec4 extra[7];
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);
	};
}