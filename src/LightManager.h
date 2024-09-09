#pragma once

#include <string>
#include <vector>

#include <glm.hpp>

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"

#include "./Core/VulkanHeader.h"
#include "./Core/ConstantBuffers.h"

#define MAX_LIGHTS 5

struct LightData {
	glm::vec4 position = glm::vec4(0.0f);
	glm::vec4 color = glm::vec4(1.0f);			// w -> light intensity
	glm::vec4 extra[9];

	glm::mat4 model = glm::mat4(1.0f);

	alignas(4) int type = 0;
	alignas(4) float scale = 0;
	alignas(4) float ambientStrength = 0.0f;
	alignas(4) float specularStrength = 0.0f;
};

namespace LightManager {
	extern Graphics::Buffer m_LightBuffer;

	void Init();
	void Shutdown();

	void AddLight(LightData& light);
	void UpdateBuffer();
	void OnUIRender();

	int GetTotalLights();

	Graphics::Buffer& GetLightBuffer();

	std::vector<LightData>& GetLights();
}
