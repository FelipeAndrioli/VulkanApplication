#version 450

#define PI 3.14159265359
#define MAX_SINE_WAVES 32

layout(quads, equal_spacing, cw) in;

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
    float tessellation_level_inner;
    float tessellation_level_outer;
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

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

layout (location = 0) in vec3 in_color[];
layout (location = 1) in vec3 in_normal[];
layout (location = 2) in vec3 in_pos[];

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_pos;
layout (location = 3) out vec3 out_frag_model_space_pos;

void main() {

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 color_bottom = mix(in_color[0], in_color[1], u);
    vec3 color_top = mix(in_color[3], in_color[2], u);
    out_color = mix(color_bottom, color_top, v);

    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    vec4 p3 = gl_in[3].gl_Position;

    vec4 pos_bottom = mix(p0, p1, u);
    vec4 pos_top = mix(p3, p2, u);
    vec4 pos = mix(pos_bottom, pos_top, v);

    float time = scene_gpu_data.time;
    float displacement_sum = 0.0;

    bool circular_waves_enabled = bool(scene_gpu_data.flags & (1 << 2));
    /*
        Book notation:

        L (w): Wavelength (w = (2 * PI) / L)
        A: Amplitude
        S (q): Wave speed (s = (S * 2 * PI) / L)
        D: wave direction
    */

    for (int wave_index = 0; wave_index < scene_gpu_data.wave_count; ++wave_index) {
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
    out_normal = vec3(0.0); 
    out_pos = vec3(push_constants.model * pos);
    out_frag_model_space_pos = pos.xyz; 
   
    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(out_pos, 1.0);

}
