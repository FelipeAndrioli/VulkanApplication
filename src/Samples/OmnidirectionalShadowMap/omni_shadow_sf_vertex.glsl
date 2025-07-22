#version 450

#extension GL_KHR_vulkan_glsl : enable

#define MAX_MODELS 5

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec3 in_tex_coord;

struct model_t {
	vec4 extra[12];
	mat4 model;
};

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int extra0;
	int extra1;
	int extra2;
	int light_far_distance;
	vec4 extra[2];
	vec4 light_position;
	mat4 view;					// view for color pass
	mat4 projection;			// projection for color pass
	mat4 light_projection;
} scene_gpu_data;

layout (std140, set = 0, binding = 1) uniform ModelGPUData {
	model_t model[MAX_MODELS];
} model_gpu_data;

layout (push_constant) uniform PushConstants {
	int model_index;
} push_constants;

// Note: Using Geometry Shader approach all matrices must be passed to the shaders at once. We migth need a Storage Buffer for that. 

void main() {
	gl_Position = model_gpu_data.model[push_constants.model_index].model * vec4(in_position, 1.0);
}