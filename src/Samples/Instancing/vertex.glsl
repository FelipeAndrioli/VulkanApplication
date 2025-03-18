#version 460

#extension GL_KHR_vulkan_glsl : enable

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	float time;					// 4
	float lightIntensity;		// 8
	int extra_2;				// 12
	int extra_3;				// 16
	vec4 lightPosition;			// 32
	vec4 lightColor;			// 48 
	vec4 extra[5];				// 128
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

struct InstanceData {
	mat4 model;
};

layout (std140, set = 0, binding = 4) readonly buffer InstanceGPUBuffer {
	InstanceData instances[];
} instanceGPUBuffer;

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

	// Change the vertex from local space, to world space (model matrix), then to view space (view matrix), then finally to clip space (projection matrix)
//	gl_Position		= sceneGPUData.proj * sceneGPUData.view * modelGPUData.model * vec4(inPosition, 1.0);
//	gl_Position		= sceneGPUData.proj * sceneGPUData.view * instanceGPUBuffer.instances[gl_BaseInstance].model * vec4(inPosition, 1.0);
	gl_Position		= sceneGPUData.proj * sceneGPUData.view * instanceGPUBuffer.instances[gl_InstanceIndex].model * vec4(inPosition, 1.0);

	// Add default vertex based color
	fragColor		= inColor;

	if (modelGPUData.flip_uv_vertically == 1) {
		fragTexCoord = vec2(inTexCoord.x, inTexCoord.y * -1);	
	} else {
		fragTexCoord = inTexCoord;	
	}

	// model normal matrix have the inverse transposed model matrix
	fragNormal		= normalize(mat3(modelGPUData.normal) * inNormal);
	fragPos			= vec3(modelGPUData.model * vec4(inPosition, 1.0));
}