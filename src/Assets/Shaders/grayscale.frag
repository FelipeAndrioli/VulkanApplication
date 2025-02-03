#version 450

layout (set = 0, binding = 0) uniform sampler2D render_target;

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 out_color;

void main() {

	vec4 color_buffer = texture(render_target, frag_tex_coord);
	vec3 gray_scale = vec3(0.5, 0.5, 0.5);

	out_color = vec4(vec3(dot(color_buffer.rgb, gray_scale)), color_buffer.a);
}
