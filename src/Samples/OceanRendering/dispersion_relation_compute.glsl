#version 450 

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (set = 0, binding = 0) uniform sampler2D wave_spectrum_image;
layout (rgba32f, binding = 1) uniform writeonly image2D dispersion_relation_ws;

void main() {

    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 image_size = textureSize(wave_spectrum_image, 0);

    if (pixel_coord.x >= image_size.x || pixel_coord.y >= image_size.y ) {
        return;
    }

    // Note: For texture fetching using integers we must use 'texelFetch'.
    vec4 wave_spectrum = texelFetch(wave_spectrum_image, pixel_coord, 0);

    imageStore(dispersion_relation_ws, pixel_coord, wave_spectrum);
}
