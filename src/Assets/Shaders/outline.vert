#version 450

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int total_lights;
	float time;
	float outline_width;
	float extra_s_3;
	vec4 camera_position;
	vec4 extra[6];
	mat4 view;
	mat4 proj;
} sceneGPUData;

layout (std140, set = 1, binding = 0) uniform ObjectGPUData {
	vec4 extra[7];
	mat4 model;
	mat4 normal_matrix;
	int extra_scalar;
	int extra_scalar1;
	int extra_scalar2;
	int flip_uv_vertically;
} objectGPUData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTexCoord;

void main() {
	vec4 pos = vec4(inPosition.xyz + inNormal * sceneGPUData.outline_width, 1.0);
	gl_Position = sceneGPUData.proj * sceneGPUData.view * objectGPUData.model * pos;
}
