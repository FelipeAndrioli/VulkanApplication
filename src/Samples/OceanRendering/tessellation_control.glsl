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
    vec4 sun;                   // xy -> pos; z -> radius; w -> strength
	vec4 viewer_position;
    vec4 water_color;           // w is empty
    vec4 local_space_camera_frustum_planes[6];
    int flags;
    int wave_count;
    int normal_wave_count;
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
    float image_width;
    float image_height;
    float fog_density;
    float fog_height_falloff;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

float calc_edge_tessellation_level(vec3 a, vec3 b) {

    if (scene_gpu_data.tessellation_max_threshold == scene_gpu_data.tessellation_min_threshold) {
        return scene_gpu_data.tessellation_level_max;
    }

    vec3 mid = 0.5 * (a + b);
    float distance_from_camera = distance(scene_gpu_data.viewer_position.xyz, mid);
    float factor = 1.0 - clamp((distance_from_camera - scene_gpu_data.tessellation_min_threshold) / (scene_gpu_data.tessellation_max_threshold - scene_gpu_data.tessellation_min_threshold), 0.0, 1.0);

    return mix(scene_gpu_data.tessellation_level_min, scene_gpu_data.tessellation_level_max, pow(factor, scene_gpu_data.tessellation_step));
}

void main() {

    bool debug_render_normals           = bool(scene_gpu_data.flags & 1);
    bool circular_waves_enabled         = bool(scene_gpu_data.flags & (1 << 1));
    bool debug_render_world_space_pos   = bool(scene_gpu_data.flags & (1 << 2));
    bool domain_warping_enabled         = bool(scene_gpu_data.flags & (1 << 3));
    bool tessellation_enabled           = bool(scene_gpu_data.flags & (1 << 4));
    bool reflection_enabled             = bool(scene_gpu_data.flags & (1 << 5));

    if (gl_InvocationID == 0) {

        bool culled = false;

        vec3 aabb_min_p = min(min(in_position[0], in_position[1]), in_position[2]);
        vec3 aabb_max_p = max(max(in_position[0], in_position[1]), in_position[2]);

        // add/subtract max height displacement here
        aabb_min_p -= vec3(5.0);
        aabb_max_p += vec3(5.0);

        for (int frustum_plane_index = 0; frustum_plane_index < 6; frustum_plane_index++) {
            vec4 plane = scene_gpu_data.local_space_camera_frustum_planes[frustum_plane_index];

            vec4 p = vec4(
                (plane.x >= 0.0) ? aabb_max_p.x : aabb_min_p.x,
                (plane.y >= 0.0) ? aabb_max_p.y : aabb_min_p.y,
                (plane.z >= 0.0) ? aabb_max_p.z : aabb_min_p.z,
                1.0
            );

            if (dot(plane, p) < 0.0) {
                culled = true;
                break;
            }
        }

        if (culled) {
            gl_TessLevelOuter[0] = 0.0;
            gl_TessLevelOuter[1] = 0.0;
            gl_TessLevelOuter[2] = 0.0;

            gl_TessLevelInner[0] = 0.0;
        } else if (tessellation_enabled) {
            vec4 p0_world_space = push_constants.model * vec4(in_position[0], 1.0);
            vec4 p1_world_space = push_constants.model * vec4(in_position[1], 1.0);
            vec4 p2_world_space = push_constants.model * vec4(in_position[2], 1.0);

            gl_TessLevelOuter[0] = calc_edge_tessellation_level(p1_world_space.xyz, p2_world_space.xyz);
            gl_TessLevelOuter[1] = calc_edge_tessellation_level(p2_world_space.xyz, p0_world_space.xyz);
            gl_TessLevelOuter[2] = calc_edge_tessellation_level(p0_world_space.xyz, p1_world_space.xyz);

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
