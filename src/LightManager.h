#pragma once

#include <string>

#include <glm.hpp>

#define MAX_LIGHTS 5

struct LightData {
	glm::vec4 position;
	glm::vec4 color;		// w -> light intensity
	glm::vec4 extra[13];

	alignas(4) int type = 0;
	alignas(4) int extra0 = 0;
	alignas(4) float ambientStrength = 0.0f;
	alignas(4) float specularStrength = 0.0f;
};

namespace Graphics {
	struct Buffer;
}

namespace LightManager {
	extern Graphics::Buffer m_LightBuffer;

	void Init();
	void Shutdown();

	void AddLight(LightData lightData);
	void UpdateBuffer();
	void OnUIRender();

	int GetTotalLights();

	Graphics::Buffer& GetLightBuffer();
}
