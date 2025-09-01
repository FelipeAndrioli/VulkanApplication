#version 450

layout (location = 0) out vec4 frag_color;

layout (push_constant) uniform PushConstants {
	mat4 model;
	vec4 color;
} push_constants;

void main() {
	frag_color = push_constants.color;
}