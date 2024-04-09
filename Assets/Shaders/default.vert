#version 450

layout (std140, set = 0, binding = 0) uniform ObjectGPUData {
	vec4 extra[12];
	mat4 model;
} objectGPUData;

layout (std140, set = 1, binding = 0) uniform SceneGPUData {
	vec4 extra[8];
	mat4 view;
	mat4 proj;
} sceneGPUData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;

void main() {
	gl_Position = sceneGPUData.proj * sceneGPUData.view * objectGPUData.model * vec4(inPosition, 1.0f);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}