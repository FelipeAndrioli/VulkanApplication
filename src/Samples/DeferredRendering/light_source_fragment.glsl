#version 450

layout (location = 0) out vec4 frag_color;

layout (push_constant) uniform PushConstants {
	mat4 model;
	vec4 light_color;
} push_constants;

void main() {
	frag_color = vec4(push_constants.light_color.rgb, 1.0);
}
