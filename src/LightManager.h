#pragma once

#include <string>

#include <glm.hpp>

#define MAX_LIGHTS 5

struct LightData {
	glm::vec4 Position;
	glm::vec4 Color;		// w -> light intensity
	glm::vec4 extra[13];

	alignas(4) int Type = 0;
	alignas(4) int Extra = 0;
	alignas(4) int Extra1 = 0;
	alignas(4) int Extra2 = 0;
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

	Graphics::Buffer& GetLightBuffer();
}
