#version 450

#define MAX_WAVES 32

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_texCoord;

layout (location = 0) out vec3 out_frag_color;
layout (location = 1) out vec3 out_frag_normal;
layout (location = 2) out vec3 out_frag_position;

layout (std140, set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
	vec4 viewer_position;
    vec4 deep_water_color;      // w is empty
    vec4 surface_water_color;   // w is fog factor height multiplier
    vec4 water_absorption;      // w is water absorption multiplier
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
    wave_data sine_wave[MAX_WAVES];
} wave_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

void main() {

    gl_Position = vec4(in_position, 1.0);

	out_frag_color	= in_color;
    out_frag_normal = in_normal;
    out_frag_position = in_position;
}
