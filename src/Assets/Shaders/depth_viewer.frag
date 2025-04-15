#version 450 core

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 in_tex_coords;
layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform sampler2D depth_map;

void main() {
	float depth = texture(depth_map, in_tex_coords).r;

	out_color = vec4(vec3(depth), 1.0);
}