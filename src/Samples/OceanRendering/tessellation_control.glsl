#version 450

#define MAX_WAVES 32

#ifdef QUAD_INTERPOLATION
#define VERTICES 4
#else
#define VERTICES 3
#endif

layout (vertices = VERTICES) out;

layout (location = 0) in vec3 in_color[];
layout (location = 1) in vec3 in_normal[];
layout (location = 2) in vec3 in_position[];

layout (location = 0) out vec3 out_color[VERTICES];
layout (location = 1) out vec3 out_normal[VERTICES];
layout (location = 2) precise out vec3 out_position[VERTICES];

layout (std140, set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
	vec4 viewer_position;
    vec4 water_color;           // w is empty
    int flags;
    int wave_count;
    float specular_displacement;
    float water_shininess;
    float temporal_phase_exponent;
    float height_multiplier;
    float wind_angle;
    float wind_speed;
    float drag_mult;
    float time;
    float water_depth;
    float sine_fbm_amplitude;
    float sine_fbm_frequency;
    float sine_fbm_amplitude_multiplier;
    float sine_fbm_frequency_multiplier;
    float tessellation_min_threshold;
    float tessellation_max_threshold;
    float tessellation_level_min;
    float tessellation_level_max;
    float tessellation_step;
    float reflection_strength;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

float calc_edge_tessellation_level(vec3 a, vec3 b) {

    vec3 mid = 0.5 * (a + b);
    float distance_from_camera = distance(scene_gpu_data.viewer_position.xyz, mid);
    float factor = 1.0 - clamp((distance_from_camera - scene_gpu_data.tessellation_min_threshold) / (scene_gpu_data.tessellation_max_threshold - scene_gpu_data.tessellation_min_threshold), 0.0, 1.0);

    return mix(factor, scene_gpu_data.tessellation_level_min, scene_gpu_data.tessellation_level_max);
}

void main() {

    bool debug_render_normals                       = bool(scene_gpu_data.flags & 1);
    bool circular_waves_enabled                     = bool(scene_gpu_data.flags & (1 << 1));
    bool debug_render_world_space_pos               = bool(scene_gpu_data.flags & (1 << 2));
    bool domain_warping_enabled                     = bool(scene_gpu_data.flags & (1 << 3));
    bool tessellation_enabled                       = bool(scene_gpu_data.flags & (1 << 4));
    bool reflection_enabled                         = bool(scene_gpu_data.flags & (1 << 5));

    if (gl_InvocationID == 0) {

        float tessellation_level_inner = 1.0;
        float tessellation_level_outer = 1.0;

        if (tessellation_enabled) {
            vec4 vertex_position = vec4(in_position[gl_InvocationID], 1.0);
            vertex_position = push_constants.model * vertex_position;

            vec4 p0 = push_constants.model * vec4(in_position[0], 1.0);
            vec4 p1 = push_constants.model * vec4(in_position[1], 1.0);
            vec4 p2 = push_constants.model * vec4(in_position[2], 1.0);

            gl_TessLevelOuter[0] = calc_edge_tessellation_level(p1.xyz, p2.xyz);
            gl_TessLevelOuter[1] = calc_edge_tessellation_level(p2.xyz, p0.xyz);
            gl_TessLevelOuter[2] = calc_edge_tessellation_level(p0.xyz, p1.xyz);

            gl_TessLevelInner[0] = max(gl_TessLevelOuter[0], max(gl_TessLevelOuter[1], gl_TessLevelOuter[2]));

        } else {
            gl_TessLevelOuter[0] = scene_gpu_data.tessellation_level_min;
            gl_TessLevelOuter[1] = scene_gpu_data.tessellation_level_min;
            gl_TessLevelOuter[2] = scene_gpu_data.tessellation_level_min;

            gl_TessLevelInner[0] = scene_gpu_data.tessellation_level_min;
        }
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_color[gl_InvocationID] = in_color[gl_InvocationID];
    out_normal[gl_InvocationID] = in_normal[gl_InvocationID];
    out_position[gl_InvocationID] = in_position[gl_InvocationID];
}
