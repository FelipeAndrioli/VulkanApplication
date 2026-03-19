#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
    mat4 extra;
    mat4 projection;
    mat4 view;
    vec4 viewer_position;
    vec4 light;
    vec4 light_view;
    int flags;
    int extra_1;
    int extra_2;
    int extra_3;
} scene_gpu_data;

layout (set = 0, binding = 1) uniform sampler2D position_buffer;
layout (set = 0, binding = 2) uniform sampler2D normal_buffer;
layout (set = 0, binding = 3) uniform sampler2D albedo_spec_buffer;
layout (set = 0, binding = 4) uniform sampler2D ssao_buffer;

layout (push_constant) uniform PushConstants {
    float specular_factor;
} push_constants;

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 frag_color;

void main() {

    vec3 frag_position = texture(position_buffer, uv).rgb;
    vec3 normal = texture(normal_buffer, uv).rgb;
    vec3 albedo = texture(albedo_spec_buffer, uv).rgb;

    float ambient_occlusion = texture(ssao_buffer, uv).r;

    bool ambient_occlusion_debug_enabled = bool(scene_gpu_data.flags & (1 << 0));

    if (ambient_occlusion_debug_enabled) {
        frag_color = vec4(ambient_occlusion.rrr, 1.0);
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

        lighting = ambient + diffuse + specular;

        frag_color = vec4(lighting, 1.0);
    }
}
