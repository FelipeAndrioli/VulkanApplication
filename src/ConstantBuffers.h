#pragma once

#include <string>

#include <glm.hpp>

struct GlobalConstants {
	float time = 0.0f;
	float extra_s_1 = 0.0f;
	float extra_s_2 = 0.0f;
	float extra_s_3 = 0.0f;
	glm::vec4 extra[7];
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);
};

struct ModelConstants {
	glm::vec4 extra[12] = {};
	glm::mat4 model = glm::mat4(1.0f);
};