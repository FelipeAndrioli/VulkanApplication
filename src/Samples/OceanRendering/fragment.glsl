#version 450

#define MAX_SINE_WAVES 32

layout (location = 0) in vec3 in_frag_color;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_pos;

layout (location = 0) out vec4 pixel_color;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
    vec4 padding[3];
	vec4 light_position;
	vec4 viewer_position;
	vec4 water_color;
    int flags;
    int sine_wave_count;
    float tessellation_level_inner;
    float tessellation_level_outer;
    float time;
    float delta_t;
} scene_gpu_data;

struct wave_data {
    vec4 direction;
    float amplitude;
    float steepness;
};

layout (std140, set = 0, binding = 1) uniform WaveGPUData {
    wave_data sine_wave[MAX_SINE_WAVES];
} wave_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

vec3 generate_normal(float x, float z, float t) {
    vec3 normal = vec3(0.0);

    for (int wave_index = 0; wave_index < scene_gpu_data.sine_wave_count; ++wave_index) {
        wave_data wave  = wave_gpu_data.sine_wave[wave_index];

        float wave_length = wave.direction.y;
        float wave_speed = wave.direction.w;
        float f = dot(wave.direction.xz, vec2(x, z)) * wave_length + t * wave_speed;
        float wave_amplitude_cos_f = wave.amplitude * cos(f);

        normal += vec3(
            -(wave_length * dot(wave.direction.xz, vec2(x, 0.0)) * wave_amplitude_cos_f), 
            1.0, 
            -(wave_length * dot(wave.direction.xz, vec2(0.0, z)) * wave_amplitude_cos_f)
        );
    }

    return normalize(normal);
}

void main() {

    bool debug_render_normals = bool(scene_gpu_data.flags & (1 << 1));
    bool generate_normal_per_fragment = bool(scene_gpu_data.flags & (1 << 2));

    vec3 normal = vec3(0.0);

    if (generate_normal_per_fragment) {
        vec3 model_space_pos = vec3(vec4(in_frag_pos, 1.0) * inverse(push_constants.model));
        vec3 model_space_normal = generate_normal(model_space_pos.x, model_space_pos.z, scene_gpu_data.time);
        normal = normalize(mat3(push_constants.model) * model_space_normal);
    } else {
        normal = in_frag_normal;
    }

    if (debug_render_normals) {
//        pixel_color = vec4(normal * 5.0, 1.0);
        pixel_color = vec4(normal, 1.0);
    } else {
        vec3 water_color = scene_gpu_data.water_color.rgb;
        float water_specular_factor = scene_gpu_data.water_color.a;

        vec3 light_dir = normalize(scene_gpu_data.light_position.xyz - in_frag_pos);
       
        float diff = max(dot(light_dir, normal), 0.1);
    
        vec3 diffuse = diff * water_color;

        vec3 view_dir = normalize(scene_gpu_data.viewer_position.xyz - in_frag_pos);
        vec3 halfway_dir = normalize(light_dir + view_dir);

        float spec = pow(max(dot(normal, halfway_dir), 0.0), water_specular_factor);
        vec3 specular = vec3(spec);

//        pixel_color = vec4(in_frag_pos, 1.0);
        pixel_color = vec4(diffuse + specular, 1.0);	
//        pixel_color = vec4(water_color, 1.0);	
    }
}
