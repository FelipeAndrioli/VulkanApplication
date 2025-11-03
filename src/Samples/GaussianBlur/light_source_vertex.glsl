#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 extra[7];
	int total_lights;
	int extra1;
	int extra2;
	float bloom_threshold;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
	vec4 color;
} push_constants;

// hardcoded cube
const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, -1.0 ),
	vec3(-1.0, 1.0, -1.0 ),
	vec3( 1.0, 1.0, -1.0 ),
	vec3( 1.0, -1.0, -1.0),

	vec3(-1.0,-1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3( 1.0,-1.0, 1.0)
);

const int indices[36] = int[36](
	// front
	0, 1, 2, 0, 2, 3,
	// right
	3, 2, 6, 3, 6, 7,
	// back
	4, 6, 5, 4, 7, 6,
	// left
	4, 5, 1, 4, 1, 0,
	// bottom
	3, 7, 4, 3, 4, 0,
	// top
	1, 5, 6, 1, 6, 2
);

void main() {
	int idx = indices[gl_VertexIndex];
	gl_Position = scene_gpu_data.projection * scene_gpu_data.view * push_constants.model * vec4(pos[idx], 1.0);
}
