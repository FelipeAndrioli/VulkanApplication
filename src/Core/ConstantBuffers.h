#pragma once

#include <string>

#include <glm.hpp>

struct GlobalConstants {
	alignas(4) int totalLights = 0;
	alignas(4) int renderNormalMap = 0;
	alignas(4) float time = 0.0f;
//	alignas(4) float minShadowBias = 0.005f;
	alignas(4) float maxShadowBias = 0.05f;
	alignas(4) float extra_s_0 = 0.0f;
	alignas(4) float extra_s_1 = 0.0f;
	alignas(4) float extra_s_2 = 0.0f;
	alignas(4) float extra_s_3 = 0.0f;
	glm::vec4 extra[14];
//	glm::vec4 cameraPosition;
//	glm::vec4 extra[6];
//	glm::mat4 view = glm::mat4(1.0f);
//	glm::mat4 proj = glm::mat4(1.0f);
};

struct ModelConstants {
	glm::vec4 extra[6] = {};
	glm::vec4 color = glm::vec4(1.0f);
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 normalMatrix = glm::mat4(1.0f);

	alignas(4) int extraScalar = 0;
	alignas(4) int extraScalar1 = 0;

	alignas(4) int flipUvVertically = 0;
	alignas(4) float outlineWidth = 0;
};

struct CameraConstants {
	glm::vec4 extra[7];
	glm::vec4 position;
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);
};
