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
	0, 2, 1, 3, 2, 0,
	// right
	1, 6, 5, 6, 1, 2,
	// back
	7, 5, 6, 5, 7, 4,
	// left
	4, 3, 0, 3, 4, 7,
	// bottom
	4, 1, 5, 1, 4, 0,
	// top
	3, 6, 2, 6, 3, 7
);

void main() {
	float cubeSize = 100.0;
	int idx = indices[gl_VertexIndex];
	gl_Position = sceneGPUData.proj * sceneGPUData.view * vec4(cubeSize * pos[idx], 1.0);
	dir = pos[idx].xyz;
}