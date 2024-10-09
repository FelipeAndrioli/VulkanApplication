#version 450

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int total_lights;
	float time;
	float extra_s_2;
	float extra_s_3;
	vec4 camera_position;
	vec4 extra[6];
	mat4 view;
	mat4 proj;
} sceneGPUData;

struct model_t {
	vec4 extra[7];
	mat4 model;
	mat4 normal_matrix;
	int extra_scalar;
	int extra_scalar1;
	
	int flip_uv_vertically;
	int outline_width;
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragTexCoord;
layout (location = 3) out vec3 fragPos;

layout (std140, set = 0, binding = 5) uniform model_uniform {
	model_t models[10];
};

layout (push_constant) uniform constant {
	int material_index;
	int model_index;
} mesh_constant;

void main() {
	model_t current_model = models[mesh_constant.model_index];

	gl_Position = sceneGPUData.proj * sceneGPUData.view * current_model.model * vec4(inPosition, 1.0);
	fragColor = inColor;

	if (current_model.flip_uv_vertically == 1) {
		fragTexCoord = vec2(inTexCoord.x, inTexCoord.y * -1);
	} else {
		fragTexCoord = inTexCoord;
	}

	fragNormal = mat3(current_model.normal_matrix) * inNormal;
	fragPos = vec3(current_model.model * vec4(inPosition, 1.0));
}

