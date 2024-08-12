#pragma once

#include "VulkanHeader.h"
#include "Assets/Camera.h"

namespace Renderer {

	void Init();
	void Destroy();

	void RenderSkybox(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera);
	void RenderMeshes(const VkCommandBuffer& commandBuffer);
	void RenderWireframe(const VkCommandBuffer& commandBuffer);

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