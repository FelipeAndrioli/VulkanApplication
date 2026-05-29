#version 450

#define PI 3.14159265359

layout(quads, equal_spacing, cw) in;

// Note: wave direction: X and Z are directions, Y is length and W is speed.
layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;
	vec4 viewer_position;
	vec4 water_color;
    vec4 wave_direction;
    int flags;
    float tessellation_level_inner;
    float tessellation_level_outer;
    float time;
    float delta_t;
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

//    vec3 normal_bottom = mix(in_normal[0], in_normal[1], u);
//    vec3 normal_top = mix(in_normal[3], in_normal[2], u);
//    out_normal = vec3(push_constants.model * vec4(mix(normal_bottom, normal_top, v), 1.0));

    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    vec4 p3 = gl_in[3].gl_Position;

    vec4 pos_bottom = mix(p0, p1, u);
    vec4 pos_top = mix(p3, p2, u);
    vec4 pos = mix(pos_bottom, pos_top, v);

    float time = scene_gpu_data.time;
   
    float wave_length = scene_gpu_data.wave_direction.y;
    float wave_speed = scene_gpu_data.wave_direction.w;

    float f = dot(scene_gpu_data.wave_direction.xz, pos.xz) * wave_length + time * wave_speed;

    pos.y = scene_gpu_data.wave_amplitude * sin(f);

//    vec3 bitangent = vec3(1.0, 0.0, wave_length * scene_gpu_data.wave_direction.x * scene_gpu_data.wave_amplitude * cos(f));
//    vec3 tangent = vec3(0.0, 1.0, wave_length * scene_gpu_data.wave_direction.y * scene_gpu_data.wave_amplitude * cos(f));
//    vec3 normal = cross(bitangent, tangent);
   
    vec3 tangent = vec3(1.0, wave_length * scene_gpu_data.wave_amplitude * cos(f), 0.0);
    vec3 normal = vec3(-tangent.y, tangent.x, 0.0);

    out_normal = normalize(mat3(push_constants.model) * normal);
    out_pos = vec3(push_constants.model * pos);
   
    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(out_pos, 1.0);

}
