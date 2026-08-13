#version 450

layout (location = 0) in vec3 light_color;
layout (location = 0) out vec4 frag_color;

layout (push_constant) uniform PushConstants {
	mat4 model;
	vec4 color;
} push_constants;

void main() {
	frag_color = vec4(light_color, 1.0);
}
