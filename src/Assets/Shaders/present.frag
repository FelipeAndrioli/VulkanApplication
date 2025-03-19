#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (set = 0, binding = 0) uniform sampler2D render_target;

layout (location = 0) in vec2 frag_tex_coord;

layout (location = 0) out vec4 out_color;

void main() {
	out_color = texture(render_target, frag_tex_coord);
}