#version 450

#extension GL_KHR_vulkan_glsl : enable


layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 view_ray;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
    mat4 projection;
    mat4 view;
    vec4 viewer_position;
    vec4 light;
    vec4 light_view;
    vec4 extras[3];
    int flags;
    float tan_half_fov;
    float aspect_ratio;
    float near_plane;
    float far_plane;
    float extra_1;
    float extra_2;
    float extra_3;
} scene_gpu_data;


void main() {
    vec2 tex_coord_pos = vec2((gl_VertexIndex << 1 & 2), gl_VertexIndex & 2);
    vec2 ray_pos = tex_coord_pos * 2 - 1;

    uv = tex_coord_pos;
    view_ray.x = -ray_pos.x * scene_gpu_data.aspect_ratio * scene_gpu_data.tan_half_fov;
    view_ray.y = ray_pos.y * scene_gpu_data.tan_half_fov;

    gl_Position = vec4(ray_pos, 0.0, 1.0);
}
