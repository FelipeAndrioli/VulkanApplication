#version 450 

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
layout (rgba32f, set = 1, binding = 2) uniform writeonly image2D dispersion_relation_ws;

#define PI 3.14159265359f
#define G 9.81f

void main() {

    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 image_size = textureSize(wave_spectrum_image, 0);

    if (pixel_coord.x >= image_size.x || pixel_coord.y >= image_size.y ) {
        return;
    }

    // temporary variables that will become part of shader input later
    float patch_size = 1000.0;

    // Note: For texture fetching using integers we must use 'texelFetch'.
    vec4 wave_spectrum = texelFetch(wave_spectrum_image, pixel_coord, 0);
    vec2 h0 = wave_spectrum.xy;
    vec2 h0_conj = wave_spectrum.zw;

    /*
       Phillips wave spctrum calculates the energy that a wave will have based 
       on its size. Long waves behaves and have different energies than short 
       waves.

       The computer works with integer and discrete indices, the loop goes from 
       x = 0 until x = dimensions - 1. However, the Phillips wave spectrum 
       formula doesn't understand what is "pixel 3" or "pixel 54". It needs to 
       know the real frequency space (the wave measurement in meters). 

       When doing:
            n = vec2(pixel_coord) - (vec2(tex_size) * 0.5);
            k = (2.0 * PI * n) / PatchSizeMeters;

        We're converting the pixel index n (e.g., (-256, 128)) into a real 
        wave vector k, measured in radians per meter. Without this 
        multiplication, Phillips wave spectrum would be calculating the energy 
        values based on pixel coordinates, which would change the ocean physical
        appearance and size if we simply changed our texture resolution which 
        we don't want.
    */
    vec2 n = vec2(pixel_coord) - (image_size * 0.5);
    vec2 k = (2.0 * PI * n) / patch_size;
    float k_length = length(k);

    if (k_length < 0.0001) {
        k_length = 0.0001;
    }

    // vector K = 2 * PI / length of the wave (patch size?)
    // g is the gravitational constant 9.8

    // dispersion relation
    // deep water where the bottom may be ignored:
    //  w~2(k) = gk
    //  w(k) = sqrt(gk)

    float W = sqrt(G * k_length);
    float phase = W * scene_gpu_data.time;

    // we can't simply use 'exp' because it would handle the exponential as a 
    // normal number instead of a complex number, therefore we need to break 
    // it down to its sin/cos equivalent and perform the multiplication 
    // ourselves.

    // e^(ix) = cos x + i sin x
    float cos_p = cos(phase);
    float sin_p = sin(phase);

    /*
        When calculating the negative exponential we have the following:
        
            e^(-ix) = cos -x + i sin -x

        Then the trigonometric circle symmetri rules its applied for negative 
        numbers.

            - The cosine is a even function: cos(-x) = cos(x). The angle sign 
            doesn't change its value (cos 30º == cos -30º).
            - The sine is a odd function: sin(-x) = -sin(x). The sine of -30º
            is the opposite of sin of 30º.
    */
    vec2 exp_pos = vec2(cos_p, sin_p);
    vec2 exp_neg = vec2(cos_p, -sin_p);

    // h0 is not a vector per se, it holds a complex number h0 (real, imaginary)
    // therefore when implementing the multiplication in the shader, we cannot 
    // use the default vec2 glsl multiplication and must expand it to complex 
    // numbers multiplication algebric expansion:
    // (a * c) + (a * id) + (ib * c) + (ib * id)
    // = ac + i(ad + bc) + i^2(bd)
    // = (ac - bd) + i(ad + bc)
    vec2 term_pos = vec2 (
        h0.x * exp_pos.x - h0.y * exp_pos.y,
        h0.x * exp_pos.y + h0.y * exp_pos.x
    );

    vec2 term_neg = vec2 (
        h0_conj.x * exp_neg.x - h0_conj.y * exp_neg.y,
        h0_conj.x * exp_neg.y + h0_conj.y * exp_neg.x
    );

    vec2 h_t = term_pos + term_neg;

    imageStore(dispersion_relation_ws, pixel_coord, vec4(h_t, 0.0, 1.0));
}
