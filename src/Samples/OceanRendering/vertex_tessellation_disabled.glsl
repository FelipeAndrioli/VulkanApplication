#version 450

#define MAX_SINE_WAVES 32

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_texCoord;

layout (location = 0) out vec3 out_frag_color;
layout (location = 1) out vec3 out_frag_normal;
layout (location = 2) out vec3 out_frag_view_space_position;
layout (location = 3) out vec3 out_frag_model_space_position;

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

// Note: wave direction: X and Z are directions, Y is length and W is speed.
// Note: circular wave: X and Y corresponds to the wave center X and Z.
struct wave_data {
    vec4 direction;
    vec4 circular_wave;
    float amplitude;
    float steepness;
};

layout (std140, set = 0, binding = 1) uniform WaveGPUData {
    wave_data sine_wave[MAX_SINE_WAVES];
} wave_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

void main() {

    vec4 pos = vec4(in_position, 1.0);

    float time = scene_gpu_data.time;
    float displacement_sum = 0.0;

    bool circular_waves_enabled = bool(scene_gpu_data.flags & (1 << 2));

    for (int wave_index = 0; wave_index < scene_gpu_data.sine_wave_count; ++wave_index) {
        wave_data wave = wave_gpu_data.sine_wave[wave_index];

        vec2 dir = vec2(0.0);

        float amplitude = wave.amplitude;
        float frequency = wave.direction.y;
        float speed = wave.direction.w;
        float steepness = wave.steepness;

        float f = 0.0;

        if (circular_waves_enabled) {
            vec2 d = pos.xz - wave.circular_wave.xy;
            float dist = length(d);

            dir = (dist > 0.0001) ? d / dist : vec2(1.0, 0.0);
            f = dist * frequency + (time * speed) * -1;
        } else {
            dir = normalize(wave.direction.xz);
            f = dot(dir, vec2(pos.xz)) * frequency + (time * speed);
        }

        float sine_base = (sin(f) + 1.0) / 2.0;
        float power_term = (sine_base > 0.0) ? pow(sine_base, steepness) : 0.0;
        float derivative = frequency * amplitude * steepness * power_term * 0.5 * cos(f);

        displacement_sum += 2 * amplitude * power_term;
    }

    pos.y = displacement_sum;

    // Note:    Normal generated on fragment shader.
	out_frag_normal = vec3(0.0);
	vec3 frag_pos = vec3(push_constants.model * pos);

	out_frag_color = in_color;
    out_frag_view_space_position = frag_pos;
    out_frag_model_space_position = pos.xyz;

    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(frag_pos, 1.0);
}
