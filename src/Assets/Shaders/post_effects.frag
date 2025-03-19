#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (set = 0, binding = 0) uniform sampler2D render_target;

layout (std140, set = 0, binding = 1) uniform PostEffectsGPUData {
	float gray_scale;
	float gamma_correction;
	int render_gray_scale;
	int render_gamma_correction;
	vec4 extra[15];
} postEffectsGPUData;

layout (location = 0) in	vec2 frag_tex_coord;
layout (location = 0) out	vec4 out_color;

vec3 gamma_correction(vec4 color_buffer) {
	return vec3(pow(color_buffer.rgb, vec3(1.0 / postEffectsGPUData.gamma_correction)));
}

vec3 gray_scale(vec3 color_buffer, vec3 gray_scale_factor) {
	return vec3(dot(color_buffer, gray_scale_factor));
}

void main() {

	vec4 color_buffer			= texture(render_target, frag_tex_coord);
	vec3 gamma_corrected_color	= postEffectsGPUData.render_gamma_correction == 1 ? gamma_correction(color_buffer) : vec3(color_buffer);
	vec3 gray_scale_factor		= vec3(postEffectsGPUData.gray_scale);
	vec3 gray_scale_color		= postEffectsGPUData.render_gray_scale == 1 ? gray_scale(gamma_corrected_color, gray_scale_factor) : gamma_corrected_color;

	out_color					= vec4(gray_scale_color, color_buffer.a);
}