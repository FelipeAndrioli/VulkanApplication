#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec2 uv;
layout (location = 1) in vec2 view_ray;

layout (location = 0) out vec4 frag_color;

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

layout (set = 0, binding = 1) uniform sampler2D depth_buffer;
layout (set = 0, binding = 2) uniform sampler2D normal_buffer;
layout (set = 0, binding = 3) uniform sampler2D albedo_spec_buffer;
layout (set = 0, binding = 4) uniform sampler2D ssao_buffer;

layout (push_constant) uniform PushConstants {
    float specular_factor;
} push_constants;

vec3 build_view_space_pos_view_ray(vec2 depth_uv, mat4 projection) {
    float depth = texture(depth_buffer, depth_uv).r * -1.0;

    depth = projection[3][2] / (depth - projection[2][2]);

    vec3 view_space_pos = vec3(view_ray * depth, depth);

    return view_space_pos;
}

vec3 build_view_space_pos_unprojection(vec2 screen_uv, vec2 depth_uv, mat4 projection) {
    float z = texture(depth_buffer, depth_uv).r;
    vec4 clip_space_pos = vec4(screen_uv * 2.0 - 1.0, z, 1.0);     // back to NDC
    vec4 view_space_pos = inverse(projection) * clip_space_pos;
    view_space_pos.xyz /= view_space_pos.w;

    return view_space_pos.xyz;
}

void main() {

    vec3 frag_position = vec3(0.0);

    vec3 normal = texture(normal_buffer, uv).rgb;
    vec3 albedo = texture(albedo_spec_buffer, uv).rgb;

    bool ambient_occlusion_debug_enabled = bool(scene_gpu_data.flags & (1 << 0));
    bool ssao_blur_enabled = bool(scene_gpu_data.flags & (1 << 1));
    bool ambient_light_only = bool(scene_gpu_data.flags & (1 << 2));
    bool debug_view_pos = bool(scene_gpu_data.flags & (1 << 3));
    bool debug_view_uv = bool(scene_gpu_data.flags & (1 << 4));
    bool build_pos_from_view_ray_enabled = bool(scene_gpu_data.flags & (1 << 5));
    bool debug_view_screen_ray = bool(scene_gpu_data.flags & (1 << 6));

    if (build_pos_from_view_ray_enabled) {
        frag_position = build_view_space_pos_view_ray(uv, scene_gpu_data.projection);
    } else {
        frag_position = build_view_space_pos_unprojection(uv, uv, scene_gpu_data.projection);
    }

    float ambient_occlusion = texture(ssao_buffer, uv).r;

    if (ambient_occlusion_debug_enabled) {
        frag_color = vec4(ambient_occlusion.rrr, 1.0);
    } else if (debug_view_pos) {
        frag_color = vec4(frag_position, 1.0);
    } else if (debug_view_uv) {
        frag_color = vec4(uv, 0.0, 1.0);
    } else if (build_pos_from_view_ray_enabled && debug_view_screen_ray) {
        frag_color = vec4(view_ray, 0.0, 1.0);
    } else {
        vec3 lighting = vec3(0.0);
        vec3 light_color = vec3(1.0);

        vec3 light_pos = scene_gpu_data.light_view.xyz;
        float light_intensity = scene_gpu_data.light.w;

        vec3 ambient = vec3(light_intensity * albedo * ambient_occlusion);

        vec3 light_dir = normalize(light_pos - frag_position);
        vec3 diffuse = max(dot(normal, light_dir), 0.0) * albedo * light_color * light_intensity;

       vec3 view_dir = normalize(-frag_position);   // view pos is (0.0.0) in view-space

        vec3 halfway_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(normal, halfway_dir), 0.0), 8.0);
        vec3 specular = vec3(light_color * spec) * vec3(push_constants.specular_factor);

        if (ambient_light_only) {
            lighting = ambient;
        } else {
            lighting = ambient + diffuse + specular;
        }

        frag_color = vec4(lighting, 1.0);
    }
}
