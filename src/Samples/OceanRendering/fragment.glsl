#version 450

#define MAX_SINE_WAVES 32

layout (location = 0) in vec3 in_frag_color;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_view_space_pos;
layout (location = 3) in vec3 in_frag_model_space_pos;

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

// Note: wave direction: X and Z are directions, Y is length and W is speed.
// Note: circular wave: X and Y corrsponds to the wave center X and Z.
struct wave_data {
    vec4 direction;
    vec4 circular_wave;
    float amplitude;
    float steepness;
};

layout (std140, set = 0, binding = 1) uniform WaveGPUData {
    wave_data sine_wave[MAX_SINE_WAVES];
} wave_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

vec3 generate_normal_old(float x, float z, float t) {
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

vec3 generate_normal(float x, float z, float t, bool circular_waves_enabled) {

    float derivative_sum_x = 0.0;
    float derivative_sum_z = 0.0;
    float time = t;

    for (int wave_index = 0; wave_index < scene_gpu_data.sine_wave_count; ++wave_index) {
        wave_data wave = wave_gpu_data.sine_wave[wave_index];

        vec2 dir = vec2(0.0);

        float amplitude = wave.amplitude;
        float frequency = wave.direction.y;
        float speed = wave.direction.w;
        float steepness = wave.steepness;

        float f = 0.0;

        if (circular_waves_enabled) {
            vec2 d = vec2(x, z) - wave.circular_wave.xy;
            float dist = length(d);

            dir = (dist > 0.0001) ? d / dist : vec2(1.0, 0.0);
            f = dist * frequency + (time * speed) * -1;
        } else {
            dir = normalize(wave.direction.xz);
            f = dot(dir, vec2(x, z)) * frequency + (time * speed);
        }
     
        float sine_base = (sin(f) + 1.0) / 2.0;
        float power_term = (sine_base > 0.0) ? pow(sine_base, steepness - 1.0) : 0.0;
        float derivative = frequency * amplitude * steepness * power_term * 0.5 * cos(f);
        float derivative_x = dir.x * derivative;
        float derivative_z = dir.y * derivative;

        derivative_sum_x += derivative_x;
        derivative_sum_z += derivative_z;
    }

    vec3 binormal = normalize(vec3(1.0, derivative_sum_x, 0.0));
    vec3 tangent = normalize(vec3(0.0, derivative_sum_z, 1.0));

    // Note: Cross product shortcut
    vec3 normal = normalize(vec3(-derivative_sum_x, 1.0, -derivative_sum_z));

//    vec3 normal = normalize(cross(tangent, binormal));

    return normal;
}

void main() {

    bool debug_render_normals = bool(scene_gpu_data.flags & (1 << 1));
    bool circular_waves_enabled = bool(scene_gpu_data.flags & (1 << 2));

    vec3 normal = generate_normal(
        in_frag_model_space_pos.x, 
        in_frag_model_space_pos.z, 
        scene_gpu_data.time, 
        circular_waves_enabled
    );

    normal = normalize(mat3(push_constants.model) * normal);

    if (debug_render_normals) {
        pixel_color = vec4(normal, 1.0);
    } else {
        vec3 water_color = scene_gpu_data.water_color.rgb;
        float water_specular_factor = scene_gpu_data.water_color.a;

        vec3 light_dir = normalize(scene_gpu_data.light_position.xyz - in_frag_view_space_pos);
      
        vec3 ambient = water_color * 0.4;
        float diff = max(dot(light_dir, normal), 0.1);
    
        vec3 diffuse = diff * water_color;

        vec3 view_dir = normalize(scene_gpu_data.viewer_position.xyz - in_frag_view_space_pos);
        vec3 halfway_dir = normalize(light_dir + view_dir);

        float spec = pow(max(dot(normal, halfway_dir), 0.0), water_specular_factor);
        vec3 specular = vec3(spec);

//        pixel_color = vec4(in_frag_view_space_pos, 1.0);
        pixel_color = vec4(ambient + diffuse + specular, 1.0);	
//        pixel_color = vec4(water_color, 1.0);	
    }
}
