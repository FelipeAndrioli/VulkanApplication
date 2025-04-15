#version 450

#define MAX_MODELS 10

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

layout (std140, set = 1, binding = 0) uniform ModelGPUData {
	vec4 extra[7];
	mat4 model;
	mat4 normal_matrix;
	int extra_scalar;
	int extra_scalar1;
	
	int flip_uv_vertically;
	float outline_width;
} modelGPUData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragPos;
layout (location = 3) out vec3 fragTangent;
layout (location = 4) out vec3 fragBiTangent;
layout (location = 5) out vec2 fragTexCoord;

/*
layout (push_constant) uniform constant {
	int material_index;
	int model_index;
	int light_source_index;
} mesh_constant;
*/

void main() {
	gl_Position = sceneGPUData.proj * sceneGPUData.view * modelGPUData.model * vec4(inPosition, 1.0);
	fragColor = inColor;

	if (modelGPUData.flip_uv_vertically == 1) {
		fragTexCoord = vec2(inTexCoord.x, inTexCoord.y * -1);
	} else {
		fragTexCoord = inTexCoord;
	}

	fragNormal = mat3(modelGPUData.normal_matrix) * inNormal;
	fragPos = vec3(modelGPUData.model * vec4(inPosition, 1.0));
}

