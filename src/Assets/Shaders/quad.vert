#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec2 frag_tex_coord;

void main() {
	frag_tex_coord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);

	gl_Position = vec4(frag_tex_coord * 2.0 - 1.0, 0.0, 1.0);
}
