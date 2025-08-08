#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_tex_coord;

struct vertex_output {
	mat3 tbn;
	vec3 light_pos;

	vec3 tangent_light_pos;
	vec3 tangent_frag_pos;
	vec3 tangent_view_pos;

	vec3 frag_pos;
	vec3 frag_normal;
	vec3 frag_color;
	vec3 frag_tangent;
	vec3 frag_bitangent;
	vec3 view_pos;
	vec2 frag_uv;
	
	float height_scale;
};

layout (location = 0) out vertex_output vt_output;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;
	vec4 view_position;
	float height_scale;
	int flags;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
	int flags;
} push_constants;

void main() {

	vec4 frag_pos = push_constants.model * vec4(in_position, 1.0);

	gl_Position = scene_gpu_data.projection * scene_gpu_data.view * frag_pos;
	
	vt_output.frag_pos			= vec3(frag_pos);
	vt_output.frag_color		= in_color;
	vt_output.light_pos			= vec3(scene_gpu_data.light_position);
	vt_output.frag_normal		= normalize(vec3(push_constants.model * vec4(in_normal, 0.0)));
	vt_output.frag_tangent		= normalize(vec3(push_constants.model * vec4(in_tangent, 0.0)));
	vt_output.frag_bitangent	= normalize(cross(in_tangent, in_normal));
	vt_output.frag_uv			= in_tex_coord;
	vt_output.view_pos			= vec3(scene_gpu_data.view_position);
	vt_output.tbn				= transpose(mat3(vt_output.frag_tangent, vt_output.frag_bitangent, vt_output.frag_normal));
	vt_output.tangent_light_pos = vt_output.tbn * vt_output.light_pos;
	vt_output.tangent_frag_pos	= vt_output.tbn * vt_output.frag_pos;
	vt_output.tangent_view_pos	= vt_output.tbn * vt_output.view_pos;
	vt_output.height_scale		= scene_gpu_data.height_scale;
}
