#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 frag_color;

layout (std140, set = 0, binding = 0) uniform PostEffectsGPUData {
    vec4 extra[15];
    int flags;
    float gamma;
    float extra2;
    float extra3;
} post_effects_gpu_data;

layout (set = 0, binding = 1) uniform sampler2D geometry_pass_result;

void main() {

    bool post_effects_enabled = bool(post_effects_gpu_data.flags & 1);
    bool gamma_correction_enabled = bool(post_effects_gpu_data.flags & (1 << 1));

    vec3 color = texture(geometry_pass_result, uv).rgb;

    if (post_effects_enabled && gamma_correction_enabled) {
        color = pow(color, vec3(1.0 / post_effects_gpu_data.gamma));
    }

    frag_color = vec4(color, 1.0); 
}
