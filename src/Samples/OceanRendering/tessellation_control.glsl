#version 450

#define MAX_SINE_WAVES 3

layout (vertices = 4) out;

layout (location = 0) in vec3 in_color[];
layout (location = 1) in vec3 in_normal[];
layout (location = 2) in vec3 in_position[];

layout (location = 0) out vec3 out_color[4];
layout (location = 1) out vec3 out_normal[4];
layout (location = 2) out vec3 out_position[4];

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
    vec4 padding[3];
	vec4 light_position;
	vec4 viewer_position;
	vec4 water_color;
    int flags;
    int sine_wave_count;
    float tessellation_level_inner;
    float tessellation_level_outer;
    float time;
    float delta_t;
} scene_gpu_data;

struct wave_data {
    vec4 direction;
    float amplitude;
};

layout (std140, set = 0, binding = 1) uniform WaveGPUData {
    wave_data sine_wave[MAX_SINE_WAVES];
} wave_gpu_data;

void main() {
    
    if (gl_InvocationID == 0) {
        /*
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
        gl_TessLevelOuter[3] = 1.0;

        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelInner[1] = 1.0;
        */
        gl_TessLevelOuter[0] = scene_gpu_data.tessellation_level_outer;
        gl_TessLevelOuter[1] = scene_gpu_data.tessellation_level_outer;
        gl_TessLevelOuter[2] = scene_gpu_data.tessellation_level_outer;
        gl_TessLevelOuter[3] = scene_gpu_data.tessellation_level_outer;

        gl_TessLevelInner[0] = scene_gpu_data.tessellation_level_inner;
        gl_TessLevelInner[1] = scene_gpu_data.tessellation_level_inner;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_color[gl_InvocationID] = in_color[gl_InvocationID];
    out_normal[gl_InvocationID] = in_normal[gl_InvocationID];
    out_position[gl_InvocationID] = in_position[gl_InvocationID];
}
