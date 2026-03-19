#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_texCoord;

layout (location = 0) out vec3 out_frag_pos;
layout (location = 1) out vec3 out_frag_normal;
layout (location = 2) out vec3 out_frag_color;
layout (location = 3) out vec3 out_frag_tangent;
layout (location = 4) out vec2 out_frag_uv;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
    mat4 extra;
    mat4 projection;
    mat4 view;
    vec4 viewer_position;
    vec4 light;
    vec4 light_view;
    int flags;
    int extra_1;
    int extra_2;
    int extra_3;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
	int material_index;
} push_constants;

void main() {

    mat3 normal_matrix = transpose(inverse(mat3(scene_gpu_data.view * push_constants.model)));

	vec4 view_pos = scene_gpu_data.view * push_constants.model * vec4(in_position, 1.0);
    vec3 normal = normal_matrix * in_normal;
	
	out_frag_pos		= view_pos.xyz;
	out_frag_color		= in_color;
	out_frag_normal		= normal.xyz;
	out_frag_uv			= in_texCoord;
	out_frag_tangent	= in_tangent; 

    gl_Position = scene_gpu_data.projection * view_pos;
}
