#version 450

layout (location = 0) out vec4 pixel_color;
layout (location = 1) out vec4 bloom_color;

layout (push_constant) uniform PushConstants {
	mat4 model;
	vec4 color;
} push_constants;

void main() {
	pixel_color = vec4(push_constants.color.rgb, 1.0);
	bloom_color = vec4(pixel_color.rgb, 1.0);
}