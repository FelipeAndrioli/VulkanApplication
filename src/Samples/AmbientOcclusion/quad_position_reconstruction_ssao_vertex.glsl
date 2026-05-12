#version 450

#extension GL_KHR_vulkan_glsl : enable


layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 view_ray;

layout (std140, set = 0, binding = 0) readonly buffer SSAOGPUData {
    mat4 projection;
    vec4 ssao_kernel[64];
    int kernel_size;
    int screen_width;
    int screen_height;
    int flags;
    float radius;
    float bias;
    float aspect_ratio;
    float tan_half_fov;
    float near_plane;
    float far_plane;
    float ssao_power;
} ssao_gpu_data;

void main() {
    vec2 tex_coord_pos = vec2((gl_VertexIndex << 1 & 2), gl_VertexIndex & 2);
    vec2 ray_pos = tex_coord_pos * 2 - 1;

    uv = tex_coord_pos;
    view_ray.x = -ray_pos.x * ssao_gpu_data.aspect_ratio * ssao_gpu_data.tan_half_fov;
    view_ray.y = ray_pos.y * ssao_gpu_data.tan_half_fov;

    gl_Position = vec4(ray_pos, 0.0, 1.0);
}
