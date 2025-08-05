#version 450

#define MAX_MODELS 10

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_texCoord;

layout (location = 0) out vec3 out_frag_color;
layout (location = 1) out vec3 out_frag_normal;
layout (location = 2) out vec3 out_frag_pos;
layout (location = 3) out vec3 out_light_pos;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

void main() {

	vec4 frag_pos = push_constants.model * vec4(in_position, 1.0);

	gl_Position = scene_gpu_data.projection * scene_gpu_data.view * frag_pos;
	
	out_frag_pos	= vec3(frag_pos);
	out_frag_color	= in_color;
	out_frag_normal = mat3(push_constants.model) * in_normal;
	out_light_pos	= vec3(scene_gpu_data.light_position);

//	fragNormal = mat3(current_model.normal_matrix) * inNormal;
}
