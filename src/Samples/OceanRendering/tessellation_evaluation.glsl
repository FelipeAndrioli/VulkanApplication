#version 450

layout(quads, equal_spacing, cw) in;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;
    float tessellation_level_inner;
    float tessellation_level_outer;
    float constant_t;
    float delta_t;
    float wave_frequency;
    float wave_amplitude;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

layout (location = 0) in vec3 in_color[];
layout (location = 1) in vec3 in_normal[];
layout (location = 2) in vec3 in_pos[];

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_pos;

void main() {

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 color_bottom = mix(in_color[0], in_color[1], u);
    vec3 color_top = mix(in_color[3], in_color[2], u);
    out_color = mix(color_bottom, color_top, v);

    vec3 normal_bottom = mix(in_normal[0], in_normal[1], u);
    vec3 normal_top = mix(in_normal[3], in_normal[2], u);
    out_normal = mix(normal_bottom, normal_top, v);

    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    vec4 p3 = gl_in[3].gl_Position;

    vec4 pos_bottom = mix(p0, p1, u);
    vec4 pos_top = mix(p3, p2, u);
    vec4 pos = mix(pos_bottom, pos_top, v);

    float distance = length(pos.xy) + 1.0;

    pos.z = sin(distance * scene_gpu_data.wave_frequency * scene_gpu_data.constant_t) * scene_gpu_data.wave_amplitude;

    out_pos = vec3(push_constants.model * pos);
   
    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(out_pos, 1.0);

}
