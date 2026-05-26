#version 450

layout (location = 0) in vec3 in_frag_color;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_pos;

layout (location = 0) out vec4 pixel_color;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;
    float tessellation_level_inner;
    float tessellation_level_outer;
    float constant_t;
    float delta_t;
    float wave_frequency;
    float wave_amplitude;
} scene_gpu_data;

void main() {

	vec3 light_dir = scene_gpu_data.light_position.xyz - in_frag_pos;

	float diff = max(dot(light_dir, in_frag_normal), 0.1);

	vec3 diffuse = diff * in_frag_color;

	pixel_color = vec4(diffuse, 1.0);	
}
