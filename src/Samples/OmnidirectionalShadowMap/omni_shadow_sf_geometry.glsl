#version 450

#extension GL_ARB_geometry_shader4 : enable

layout (triangles) in;
layout (triangle_strip, max_vertices = 3 * 6) out;

layout (location = 0) out vec4 out_frag_world_position;
layout (location = 1) out vec4 out_light_world_position;
layout (location = 2) out int out_light_far_distance;

layout (set = 0, binding = 0) uniform SceneGPUData {
	int extra0;
	int extra1;
	int extra2;
	int light_far_distance;
	vec4 extra[2];
	vec4 light_position;
	mat4 view;					// view for color pass
	mat4 projection;			// projection for color pass
	mat4 light_projection;
} scene_gpu_data;

layout (set = 0, binding = 2) readonly buffer StorageBuffer {
	mat4 view[6];
} storage_buffer;

void main() {

	// Note: All variables are reset after emitting a vertex, therefore all of them must be set again.

	for (int LightFaceIndex = 0; LightFaceIndex < 6; ++LightFaceIndex) {
		for (int VertexIndex = 0; VertexIndex < 3; ++VertexIndex) {

			out_light_far_distance		= scene_gpu_data.light_far_distance;
			out_light_world_position	= scene_gpu_data.light_position;
			out_frag_world_position		= gl_in[VertexIndex].gl_Position;

			gl_Position = scene_gpu_data.light_projection * storage_buffer.view[LightFaceIndex] * gl_in[VertexIndex].gl_Position;
			gl_Layer	= LightFaceIndex;

			EmitVertex();
		}

		EndPrimitive();
	}
}