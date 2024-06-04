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

layout (location = 0) out vec3 dir;

// hardcoded cube
const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),
	vec3( 1.0,-1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),

	vec3(-1.0,-1.0,-1.0),
	vec3( 1.0,-1.0,-1.0),
	vec3( 1.0, 1.0,-1.0),
	vec3(-1.0, 1.0,-1.0)
);

const int indices[36] = int[36](
	// front
	0, 1, 2, 2, 3, 0,
	// right
	1, 5, 6, 6, 2, 1,
	// back
	7, 6, 5, 5, 4, 7,
	// left
	4, 0, 3, 3, 7, 4,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
);

void main() {
	float cubeSize = 100.0;
	int idx = indices[gl_VertexIndex];
	gl_Position = sceneGPUData.proj * sceneGPUData.view * objectGPUData.model * vec4(cubeSize * pos[idx], 1.0);
	dir = pos[idx].xyz;
}