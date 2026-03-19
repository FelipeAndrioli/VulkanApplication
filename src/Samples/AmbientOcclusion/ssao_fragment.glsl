#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 uv;

layout (location = 0) out float frag_color;

layout (std140, set = 0, binding = 0) readonly buffer SSAOGPUData {
    mat4 projection;
    vec4 ssao_kernel[64];
    int kernel_size;
    int screen_width;
    int screen_height;
    int flags;
    float radius;
    float bias;
} ssao_gpu_data;

layout (set = 0, binding = 1) uniform sampler2D position;
layout (set = 0, binding = 2) uniform sampler2D normal;
layout (set = 0, binding = 3) uniform sampler2D albedo_spec;
layout (set = 0, binding = 4) uniform sampler2D ssao_noise;

void main() {

    int mask = (1 << 0);

    bool range_check_enabled = bool(ssao_gpu_data.flags & mask);

    // tile noise texture over the screen, based on screen dimensions 
    // divided by noise size.
    vec2 noise_scale = vec2(ssao_gpu_data.screen_width / 4.0, ssao_gpu_data.screen_height / 4.0);

    vec3 frag_pos = texture(position, uv).rgb;
    vec3 frag_normal = normalize(texture(normal, uv).rgb);
    vec3 frag_albedo_spec = texture(albedo_spec, uv).rgb;
    vec3 random_vec = texture(ssao_noise, uv * noise_scale).rgb;

    // the ssao kernel is a vector created at tangent space, it has to be 
    // converted to view space

    // Using a process called the Gramm-Schmidt process we create an 
    // orthogonal basis, each time slightly tilted based on the value of 
    // random_vec. Because we use a random vector for constructing the tangent
    // vector, there is no need to have the TBN matrix exactly aligned to the
    // geometry's surface, thus no need for per-vertex tangent (and bitangent)
    // vectors.
    vec3 tangent = normalize(random_vec - frag_normal * dot(random_vec, frag_normal));
    vec3 bitangent = cross(frag_normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, frag_normal);

    float occlusion = 0.0;

    for (int i = 0; i < ssao_gpu_data.kernel_size; i++) {
        // For each iteration we first transform the respective sample to
        // view-space. We then add the view-space kernel offset to the 
        // view-space fragment position. Then we multiply the offset sample by
        // radius to increase (or decrease) the effective sample radius of 
        // SSAO.
        vec3 sample_pos = TBN * ssao_gpu_data.ssao_kernel[i].xyz;
        sample_pos = frag_pos + sample_pos * ssao_gpu_data.radius;

        vec4 offset = vec4(sample_pos, 1.0);
        offset = ssao_gpu_data.projection * offset;     // from view to clip-space
        offset.xyz /= offset.w;                         // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5;            // transform to range 0.0 - 1.0

        float range_check = 1.0;
        float sample_depth = texture(position, offset.xy).z;

        if (range_check_enabled) {
            range_check = smoothstep(0.0, 1.0, ssao_gpu_data.radius / abs(frag_pos.z - sample_depth));
        }

        occlusion += (sample_depth >= sample_pos.z + ssao_gpu_data.bias ? 1.0 : 0.0) * range_check;
    }

    occlusion = 1.0 - (occlusion / ssao_gpu_data.kernel_size);

    frag_color = occlusion;
}
