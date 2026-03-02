#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 frag_color;

layout (std140, set = 0, binding = 0) uniform PostEffectsGPUData {
    vec4 extra[15];
    float gamma;
    float extra1;
    float extra2;
    float extra3;
} post_effects_gpu_data;

layout (set = 0, binding = 1) uniform sampler2D geometry_pass_result;

void main() {
    vec3 color = texture(geometry_pass_result, uv).rgb;
    color = pow(color, vec3(1.0 / post_effects_gpu_data.gamma));

    frag_color = vec4(color, 1.0); 
}
