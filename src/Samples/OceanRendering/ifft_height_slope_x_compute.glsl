#version 450

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (set = 1, binding = 0) uniform sampler2D wave_spectrum;
layout (set = 1, binding = 1) uniform sampler2D dispersion_relation;
layout (set = 1, binding = 2) uniform sampler2D dispersion_relation_slope;
layout (set = 1, binding = 3) uniform sampler2D source_image;
layout (rgba32f, set = 1, binding = 4) uniform writeonly image2D target_image;

layout (push_constant) uniform PushConstants {
	mat4 model;
    vec4 color;
    int stage;
    int flags;
    float pi_over_L;
    float patch_size;
} ifft_push_constants;

#define PI 3.14159265359f

void main() {

    bool first_pass = bool(ifft_push_constants.flags & 1);
    bool last_pass  = bool(ifft_push_constants.flags & (1 << 1));
    bool vertical   = bool(ifft_push_constants.flags & (1 << 2));

    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
   
    int N = imageSize(target_image).x;

    if (uv.x >= N || uv.y >= N) {
        return;
    }

    // Current block size
    int L = 1 << ifft_push_constants.stage;

    // Stockham index sorting calculations 
    int coordinate_x = vertical ? uv.y : uv.x;
    int current_line = vertical ? uv.x : uv.y;

    int j = coordinate_x % L;
//    int k = coordinate_x / (2 * L);
    int k = coordinate_x >> (ifft_push_constants.stage + 1);    // optimization alternative of above line

    // locate the two dual inputs (butterflies) from the source texture 
    int src_x0 = k * L + j;
    int src_x1 = src_x0 + (N / 2);

    // Determines the correct coordinate to read from source textures
    ivec2 read_uv0 = vertical ? ivec2(current_line, src_x0) : ivec2(src_x0, current_line);
    ivec2 read_uv1 = vertical ? ivec2(current_line, src_x1) : ivec2(src_x1, current_line);

    // Read the complex values (x - real, y - imaginary) 
    vec4 val0;
    vec4 val1;

    if (first_pass) {
        val0.rg = texelFetch(dispersion_relation, read_uv0, 0).rg;
        val1.rg = texelFetch(dispersion_relation, read_uv1, 0).rg;

        // dispersion_relation_slope rg -> Slope X
        // dispersion_relation_slope ba -> Slope Y
        val0.ba = texelFetch(dispersion_relation_slope, read_uv0, 0).rg;
        val1.ba = texelFetch(dispersion_relation_slope, read_uv1, 0).rg;

    } else {
        val0 = texelFetch(source_image, read_uv0, 0);
        val1 = texelFetch(source_image, read_uv1, 0);
    }

    // compute twiddle factor for IFFT (positive exponent angle)
//    float angle = PI * float(j) / float(L);
    float angle = ifft_push_constants.pi_over_L * float(j);
    vec2 twiddle = vec2(cos(angle), sin(angle));

    // complex multiplication: val1 * twiddle
    vec4 twiddled_val1;

    // height (xy)
    twiddled_val1.x = val1.x * twiddle.x - val1.y * twiddle.y;
    twiddled_val1.y = val1.x * twiddle.y + val1.y * twiddle.x;

    // slope x (zw)
    twiddled_val1.z = val1.z * twiddle.x - val1.w * twiddle.y;
    twiddled_val1.w = val1.z * twiddle.y + val1.w * twiddle.x;

    // out of place butterfly addition/subtraction 
    vec4 result = vec4(0.0);

    // optimization alternative for division by L
    bool is_even = ((coordinate_x >> ifft_push_constants.stage) & 1) == 0;

//    if ((coordinate_x / L) % 2 == 0) {
    if (is_even) {
        // even indices get the sum 
        result = val0 + twiddled_val1;
    } else {
        // odd indices get the difference
        result = val0 - twiddled_val1;
    }

    if (last_pass) {
        result *= (1.0 / float(N * N)); 
   
        float final_height = result.x;
        float final_slope_x = result.z;
        
        imageStore(target_image, uv, vec4(final_height, final_slope_x, 0.0, 1.0));
    } else {
        imageStore(target_image, uv, result);
    }
}
