#version 450

#extension GL_KHR_vulkan_glsl : enable

#define MAX_MODELS 5

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_tex_coord;

struct model_t {
	vec4 extra[12];
	mat4 model;
};

layout (location = 0) out vec4 frag_world_position;	
layout (location = 1) out vec4 light_position; 
layout (location = 2) out vec4 view_position;
layout (location = 3) out vec3 frag_normal;
layout (location = 4) out int light_far_distance;
layout (location = 5) out int flags;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int extra0;
	int extra1;
	int flags;
	int light_far_distance;
	vec4 extra;
	vec4 view_position;
	vec4 light_position;
	mat4 view;
	mat4 projection;
	mat4 light_projection;
} scene_gpu_data;

layout (std140, set = 0, binding = 1) uniform ModelGPUData {
	model_t model[MAX_MODELS];
} model_gpu_data;

layout (push_constant) uniform ScenePushConstants {
	int model_index;
} scene_push_constants;

void main() {
	frag_world_position	= model_gpu_data.model[scene_push_constants.model_index].model * vec4(in_position, 1.0);
	light_position		= scene_gpu_data.light_position;
	frag_normal			= normalize(mat3(model_gpu_data.model[scene_push_constants.model_index].model) * in_normal);
	light_far_distance	= scene_gpu_data.light_far_distance;
	view_position		= scene_gpu_data.view_position;
	flags				= scene_gpu_data.flags;

	gl_Position = scene_gpu_data.projection * scene_gpu_data.view * frag_world_position;
}