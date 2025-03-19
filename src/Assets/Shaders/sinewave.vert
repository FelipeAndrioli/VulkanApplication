#version 450

layout (std140, set = 0, binding = 0) uniform ObjectGPUData {
	vec4 extra[12];
	mat4 model;
} objectGPUData;

layout (std140, set = 1, binding = 0) uniform SceneGPUData {
	float time;
	float extra_s_1;
	float extra_s_2;
	float extra_s_3;
	vec4 extra[7];
	mat4 view;
	mat4 proj;
} sceneGPUData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;

void main() {
	vec3 vertex_position = inPosition;

	float amplitude = 0.5f;
	float wavelength = 2.0f;
	float frequency = 2 / wavelength;
	float speed = 0.5f;
	float desired_speed = speed * (2 / wavelength);

	vertex_position.y = amplitude * sin(vertex_position.x * frequency + sceneGPUData.time * desired_speed);

	gl_Position = sceneGPUData.proj * sceneGPUData.view * objectGPUData.model * vec4(vertex_position, 1.0f);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}