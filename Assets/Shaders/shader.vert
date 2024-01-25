#version 450

layout (set = 0, binding = 0) uniform SceneGPUData {
	mat4 view;
	mat4 proj;
} sceneGPUData;

layout (set = 1, binding = 0) uniform ObjectGPUData {
	mat4 model;
} objectGPUData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;

void main() {
	gl_Position = sceneGPUData.proj * sceneGPUData.view * objectGPUData.model * vec4(inPosition, 1.0);
}