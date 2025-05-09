#version 450

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	float time;					// 4
	float explode;				// 8
	int extra_2;				// 12
	int extra_3;				// 16
	vec4 extra[7];				// 128
	mat4 view;					// 192
	mat4 proj;					// 256
} sceneGPUData;

layout (std140, set = 0, binding = 1) uniform ModelGPUData {
	int flip_uv_vertically;		// 4
	int extra_1;				// 8
	int extra_2;				// 12
	int extra_3;				// 16
	vec4 extra[7];				// 128
	mat4 model;					// 192
	mat4 normal;				// 256
} modelGPUData;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec2 inTexCoord;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragColor;
layout (location = 3) out vec3 fragTangent;
layout (location = 4) out vec2 fragTexCoord;

void main() {
	gl_Position = sceneGPUData.view * modelGPUData.model * vec4(inPosition, 1.0);
	
	fragColor = inColor;

	if (modelGPUData.flip_uv_vertically == 1) {
		fragTexCoord = vec2(inTexCoord.x, inTexCoord.y * -1);	
	} else {
		fragTexCoord = inTexCoord;	
	}

	fragNormal = normalize(vec3(vec4(mat3(modelGPUData.normal) * inNormal, 0.0)));
}
