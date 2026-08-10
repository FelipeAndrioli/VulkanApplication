#version 450

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 frag_color;

layout (set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
    vec4 sun;                   // xy -> pos; z -> radius; w -> strength
	vec4 viewer_position;
    vec4 water_color;           // w is empty
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

void main() {

    // Move light pos from [-1, 1] to [0, 1]
    vec2 light_pos = scene_gpu_data.sun.xy * 0.5 + 0.5;

    // calculate aspect correction for a circular sun
    vec2 aspect_correction = vec2(scene_gpu_data.image_width / scene_gpu_data.image_height, 1.);

    float dist = clamp(length((uv - light_pos) * aspect_correction), 0.0, 1.0);
    float sun_radius = scene_gpu_data.sun.z;
    float sun_strength = scene_gpu_data.sun.w; 
    float sun_mask = smoothstep(sun_radius, sun_radius - 0.05, dist);

    vec3 sun_color = scene_gpu_data.light_color.rgb * sun_strength * sun_mask;

    frag_color.rgb = sun_color;
}
