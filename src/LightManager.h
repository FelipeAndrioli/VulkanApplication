#pragma once

#include <string>
#include <vector>

#include <glm.hpp>

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"

#include "./Core/VulkanHeader.h"
#include "./Core/ConstantBuffers.h"

#define MAX_LIGHTS 5

enum LightType {
	Undefined = -1,
	Directional = 0,
	PointLight = 1,
	SpotLight = 2
};

struct LightData {
	glm::vec4 position = glm::vec4(0.0f);
	glm::vec4 direction = glm::vec4(0.0f);		// w -> processed cutoff angle
	glm::vec4 color = glm::vec4(1.0f);			// w -> light intensity
	glm::vec4 extra[7];

	glm::mat4 model = glm::mat4(1.0f);

	alignas(4) LightType type = LightType::Undefined;

	alignas(4) float cutOffAngle = 0.0f;		// raw cutoff angle
	alignas(4) float linearAttenuation = 0.0f;
	alignas(4) float quadraticAttenuation = 0.0f;
	alignas(4) float scale = 0.0f;
	alignas(4) float ambient = 0.0f;
	alignas(4) float diffuse = 0.0f;
	alignas(4) float specular = 0.0f;
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
