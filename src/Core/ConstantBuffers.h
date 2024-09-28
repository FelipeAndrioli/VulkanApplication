#pragma once

#include <string>

#include <glm.hpp>

struct GlobalConstants {
	alignas(4) int totalLights = 0;
	alignas(4) float time = 0.0f;
	alignas(4) float extra_s_2 = 0.0f;
	alignas(4) float extra_s_3 = 0.0f;
	glm::vec4 cameraPosition;
	glm::vec4 extra[6];
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);
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
