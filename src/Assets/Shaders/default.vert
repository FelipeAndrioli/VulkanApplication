#version 450

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	float time;
	float extra_s_1;
	float extra_s_2;
	float extra_s_3;
	vec4 extra[7];
	mat4 view;
	mat4 proj;
} sceneGPUData;

layout (std140, set = 1, binding = 0) uniform ObjectGPUData {
	vec4 extra[11];
	mat4 model;
	int extra_scalar;
	int extra_scalar1;
	int extra_scalar2;
	int flip_uv_vertically;
} objectGPUData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec2 fragTexCoord;
layout (location = 3) out vec3 fragPos;

void main() {
	gl_Position = sceneGPUData.proj * sceneGPUData.view * objectGPUData.model * vec4(inPosition, 1.0f);
	fragColor = inColor;

	if (objectGPUData.flip_uv_vertically == 1) {
		fragTexCoord = vec2(inTexCoord.x, inTexCoord.y * -1);
	} else {
		fragTexCoord = inTexCoord;
	}

	fragNormal = inNormal;
	fragPos = vec3(objectGPUData.model * vec4(inPosition, 1.0f));
}

