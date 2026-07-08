#version 450

#define MAX_SINE_WAVES 32

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
	vec4 light_position;
	vec4 light_color;   // w is specular
	vec4 viewer_position;
	vec4 deep_water_color;
	vec4 shallow_water_color;
    int flags;
    int wave_count;
    float shallow_water_color_sum_deviation;
    float deep_water_color_sum_deviation;
    float time;
    float water_depth;
    float sine_fbm_amplitude;
    float sine_fbm_frequency;
    float sine_fbm_amplitude_multiplier;
    float sine_fbm_frequency_multiplier;
} scene_gpu_data;

// Note: wave direction: X and Z are directions, Y is length and W is speed.
// Note: circular wave: X and Y corrsponds to the wave center X and Z.
struct wave_data {
    vec4 direction;
    vec4 circular_wave;
    float amplitude;
    float steepness;
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
        gl_TessLevelOuter[0] = 64.0;
        gl_TessLevelOuter[1] = 64.0;
        gl_TessLevelOuter[2] =  64.0;
        gl_TessLevelOuter[3] =  64.0;

        gl_TessLevelInner[0] =  64.0;
        gl_TessLevelInner[1] =  64.0;

    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_color[gl_InvocationID] = in_color[gl_InvocationID];
    out_normal[gl_InvocationID] = in_normal[gl_InvocationID];
    out_position[gl_InvocationID] = in_position[gl_InvocationID];
}
