#version 450

#extension GL_KHR_vulkan_glsl : enable

#define MAX_MODELS 5

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_tex_coord;

layout (location = 0) out vec4 out_frag_world_position;
layout (location = 1) out vec4 out_light_position;
layout (location = 2) out int out_light_far_distance;

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
	mat4 view;					// view for geometry pass
	mat4 projection;			// projection for geometry pass
	mat4 light_projection;
} scene_gpu_data;

layout (std140, set = 0, binding = 1) uniform ModelGPUData {
	model_t model[MAX_MODELS];
} model_gpu_data;

layout (push_constant) uniform PushConstants {
	int extra1;
	int extra2;
	int extra3;
	int model_index;
	mat4 view;
} push_constants;

void main() {
	out_frag_world_position = model_gpu_data.model[push_constants.model_index].model * vec4(in_position, 1.0);
	out_light_position = scene_gpu_data.light_position;
	out_light_far_distance = scene_gpu_data.light_far_distance;

	gl_Position = scene_gpu_data.light_projection * push_constants.view * out_frag_world_position;
}
