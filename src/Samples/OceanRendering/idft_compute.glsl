#version 450 

#define PI 3.14159265359

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (std140, set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
    vec4 sun;                   // xy -> pos; z -> radius; w -> strength
	vec4 viewer_position;
    vec4 water_color;           // w is empty
    vec4 displacement;
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

layout (set = 1, binding = 0) uniform wave_spectrum_parameters {
    mat4 extra[3];
    vec4 extra1[2];
    vec2 wind_direction;
    int dimension;
    float patch_size;
    float wind_speed;
    float amplitude;
    float small_wave_suppression_threshold;
} wave_parameters;

layout (set = 1, binding = 1) uniform sampler2D wave_spectrum_image;
//layout (rgba32f, set = 1, binding = 2) uniform writeonly image2D dispersion_relation_ws_storage;
layout (set = 1, binding = 3) uniform sampler2D dispersion_relation_ws;
layout (rgba32f, set = 1, binding = 4) uniform writeonly image2D idft_result;

void main() {

    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 image_size = textureSize(dispersion_relation_ws, 0);

    if (pixel_coord.x >= image_size.x || pixel_coord.y >= image_size.y) {
        return;
    }

    // Note: Assumption of a square image.
    int sample_count = int(image_size.x);

    float inverse_total_samples = 1.0 / float(sample_count * sample_count);

    float accumulator_height = 0.0;
    float accumulator_slope_x = 0.0;
    float accumulator_slope_y = 0.0;

    for (int frequency_domain_horizontal_index = 0; frequency_domain_horizontal_index < sample_count; frequency_domain_horizontal_index++) {
        for (int frequency_domain_vertical_index = 0; frequency_domain_vertical_index < sample_count; frequency_domain_vertical_index++) {
            ivec2 fetching_coords = ivec2(frequency_domain_horizontal_index, frequency_domain_vertical_index);

            vec2 frequency_domain_sample = texelFetch(dispersion_relation_ws, fetching_coords, 0).xy;

            float real = frequency_domain_sample.x;
            float imaginary = frequency_domain_sample.y;

            float u = float(frequency_domain_horizontal_index) / float(sample_count);
            float v = float(frequency_domain_vertical_index) / float(sample_count);

            float angle = 2.0 * PI * (float(pixel_coord.x) * u + float(pixel_coord.y) * v);

            float cos_a = cos(angle);
            float sin_a = sin(angle);

            accumulator_height += (real * cos_a) - (imaginary * sin_a);

            float slope_base = (imaginary * cos_a) + (real * sin_a);

            float kx = float (frequency_domain_horizontal_index >= sample_count / 2.0 ? frequency_domain_horizontal_index - sample_count : frequency_domain_horizontal_index);
            float ky = float (frequency_domain_vertical_index >= sample_count / 2.0 ? frequency_domain_vertical_index - sample_count : frequency_domain_vertical_index);

            accumulator_slope_x -= kx * slope_base;
            accumulator_slope_y -= ky * slope_base;
        }
    }

    float wave_height = accumulator_height * inverse_total_samples;
    float wave_slope_x = accumulator_slope_x * inverse_total_samples;
    float wave_slope_y = accumulator_slope_y * inverse_total_samples;

    imageStore(idft_result, pixel_coord, vec4(wave_height, wave_slope_x, wave_slope_y, 1.0));
}
