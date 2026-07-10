#version 450

#define MAX_WAVES 32

layout (location = 0) in vec3 in_frag_color;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_world_space_pos;
layout (location = 3) in vec3 in_frag_model_space_pos;
layout (location = 4) in vec3 in_frag_original_model_space_pos;

layout (location = 0) out vec4 pixel_color;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;    // w is light strength
	vec4 light_color;       // w is light specular
	vec4 viewer_position;
    int flags;
    int wave_count;
    float specular_displacement;
    float water_shininess;
    float temporal_phase_exponent;
    float height_multiplier;
    float wind_angle;
    float wind_speed;
    float drag_mult;
    float time;
    float water_depth;
    float sine_fbm_amplitude;
    float sine_fbm_frequency;
    float sine_fbm_amplitude_multiplier;
    float sine_fbm_frequency_multiplier;
    float padding1;
    float padding2;
    float padding3;
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
    wave_data wave[MAX_WAVES];
} wave_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

struct wave_function_result {
    vec3 position;
    vec3 normal;
    float displacement_sum;
    float derivative_sum_x;
    float derivative_sum_z;
};

wave_function_result sine_wave(vec4 pos, float time, bool circular_waves_enabled) {
    float displacement_sum = 0.0;
    float derivative_sum_x = 0.0;
    float derivative_sum_z = 0.0;

    for (int wave_index = 0; wave_index < scene_gpu_data.wave_count; ++wave_index) {
        wave_data wave = wave_gpu_data.wave[wave_index];

        vec2 dir = vec2(0.0);

        float amplitude = wave.amplitude;
        float frequency = wave.direction.y;
        float speed = wave.direction.w;
        float steepness = wave.steepness;

        float f = 0.0;

        if (circular_waves_enabled) {
            vec2 d = pos.xz - wave.circular_wave.xy;
            float dist = length(d);

            // "manual" normalization to avoid division very close to 0.
            dir = (dist > 0.0001) ? d / dist : vec2(1.0, 0.0);
            f = dist * frequency + (time * speed) * -1;
        } else {
            dir = normalize(wave.direction.xz);
            f = dot(dir, vec2(pos.xz)) * frequency + (time * speed);
        }

        float sine_base = (sin(f) + 1.0) / 2.0;

        // Check on sine base greater than zero to skip a pow of 0.
        float power_term = (sine_base > 0.0) ? pow(sine_base, steepness) : 0.0;
        float normal_power_term = (sine_base > 0.0) ? pow(sine_base, steepness - 1.0) : 0.0;

        // The book height function compresses the sine wave range by / 2, its
        // rate of change is halved. Adding the 0.5 factor synchronizes the
        // derivative with the actual height change of the vertices.
        float derivative = frequency * amplitude * steepness * power_term * 0.5 * cos(f);
        float normal_derivative = frequency * amplitude * steepness * normal_power_term * 0.5 * cos(f);

        float derivative_x = dir.x * normal_derivative;
        float derivative_z = dir.y * normal_derivative;

        displacement_sum += 2 * amplitude * power_term;
        derivative_sum_x += derivative_x;
        derivative_sum_z += derivative_z;
    }

    vec3 binormal = normalize(vec3(1.0, derivative_sum_x, 0.0));
    vec3 tangent = normalize(vec3(0.0, derivative_sum_z, 1.0));

    // Note: Cross product shortcut
    vec3 normal = normalize(vec3(-derivative_sum_x, 1.0, -derivative_sum_z));
//    vec3 normal = normalize(cross(tangent, binormal));

    wave_function_result result;

    result.normal = normal;
    result.displacement_sum = displacement_sum;
    result.derivative_sum_x = derivative_sum_x;
    result.derivative_sum_z = derivative_sum_z;

    return result;
}

wave_function_result gerstner_wave(vec4 pos, float time, bool circular_waves_enabled) {

    float pos_sum_x = 0.0;
    float pos_sum_y = 0.0;
    float pos_sum_z = 0.0;

    vec3 normal = vec3(0.0);
    vec3 tangent = vec3(0.0);
    vec3 binormal = vec3(0.0);

    for (int wave_index = 0; wave_index < scene_gpu_data.wave_count; ++wave_index) {
        wave_data wave = wave_gpu_data.wave[wave_index];

        float amplitude = wave.amplitude;
        float frequency = wave.direction.y;
        float speed = wave.direction.w;
        float steepness = wave.steepness / (frequency * amplitude * scene_gpu_data.wave_count);

        vec2 dir = vec2(0.0);

        float f = 0.0;

        if (circular_waves_enabled) {
            vec2 d = pos.xz - wave.circular_wave.xy;
            float dist = length(d);

            // "manual" normalization to avoid division very close to 0.
            dir = (dist > 0.0001) ? d / dist : vec2(1.0, 0.0);
            f = frequency * dist + (time * speed);
        } else {
            dir = normalize(wave.direction.xz);
            f = dot(dir, vec2(pos.x, pos.z)) * frequency + (time * speed);
        }

        float pos_x = steepness * amplitude * dir.x * cos(f);
        float pos_z = steepness * amplitude * dir.y * cos(f);
        float pos_y = amplitude * sin(f);
   
        pos_sum_x += pos_x;
        pos_sum_y += pos_y;
        pos_sum_z += pos_z;

        tangent.x += steepness * dir.x * dir.x * frequency * amplitude * sin(f);
        tangent.y += dir.x * frequency * amplitude * cos(f);
        tangent.z += steepness * dir.x * dir.y * frequency * amplitude * sin(f);

        binormal.x += steepness * dir.x * dir.y * frequency * amplitude * sin(f);
        binormal.y += dir.y * frequency * amplitude * cos(f);
        binormal.z += steepness * dir.y * dir.y * frequency * amplitude * sin(f);

        /*
        vec3 current_wave_normal = vec3(0.0);
        current_wave_normal.x = dir.x * frequency * amplitude * cos(f);
        current_wave_normal.y = steepness * frequency * amplitude * sin(f);
        current_wave_normal.z = dir.y * frequency * amplitude * cos(f);

//        normal += vec3(current_wave_normal.x * -1.0, 1.0 - current_wave_normal.y, current_wave_normal.z * -1.0);
        normal += vec3(current_wave_normal.x, current_wave_normal.y, current_wave_normal.z);
        */
    }

    /*
    normal.x = -normal.x;
    normal.y = 1.0 - normal.y;
    normal.z = -normal.z;
    */

    tangent.x = 1.0 - tangent.x;
    tangent.z = -tangent.z;

    binormal.x = -binormal.x;
    binormal.z = 1.0 - binormal.z;

    normal = normalize(cross(binormal, tangent));

    wave_function_result result;
    result.position = vec3(pos.x + pos_sum_x, pos_sum_y, pos.z + pos_sum_z);
    result.normal = normal; 

    return result;
}

void main() {

    bool sine_waves_enabled                         = bool(scene_gpu_data.flags & 1);
    bool debug_render_normals                       = bool(scene_gpu_data.flags & (1 << 1));
    bool circular_waves_enabled                     = bool(scene_gpu_data.flags & (1 << 2));
    bool gerstner_waves_enabled                     = bool(scene_gpu_data.flags & (1 << 3));
    bool debug_render_world_space_pos               = bool(scene_gpu_data.flags & (1 << 4));
    bool sine_wave_fractal_brownian_motion_enabled  = bool(scene_gpu_data.flags & (1 << 5));
    bool domain_warping_enabled                     = bool(scene_gpu_data.flags & (1 << 6));

    float time = scene_gpu_data.time;
    vec4 pos = vec4(in_frag_model_space_pos, 1.0);

    vec3 normal = vec3(0.0);

    if (gerstner_waves_enabled) {
        wave_function_result w = gerstner_wave(pos, time, circular_waves_enabled);
        normal = normalize(mat3(push_constants.model) * w.normal);
    } 

    if (sine_waves_enabled) {
        wave_function_result w = sine_wave(pos, time, circular_waves_enabled);
        normal = normalize(mat3(push_constants.model) * w.normal);
    }

    if (!gerstner_waves_enabled && !sine_waves_enabled && sine_wave_fractal_brownian_motion_enabled) {
        normal = in_frag_normal;
    }

    if (debug_render_normals) {
        pixel_color = vec4(normal * 5.0, 1.0);
    } else {

        //        vec3 deep_water_color = vec3(0.0293, 0.0698, 0.1717);
        //        vec3 shallow_water_color = vec3(0.1529, 0.8901, 0.8392);
        /*
        vec3 deep_water_color = vec3(0.09, 0.102, 0.31);
        vec3 shallow_water_color = vec3(0., 0.957, 1.);

        float water_color_deviation = 0.550; 
        //        float water_color_deviation = 1.;

        float water_height_factor = (pos.y - scene_gpu_data.water_depth * -1.0) / (scene_gpu_data.water_depth - scene_gpu_data.water_depth * -1.0);
        water_height_factor = clamp(water_height_factor, 0.0, 1.0);

        vec3 water_color = mix(deep_water_color, shallow_water_color, water_height_factor * water_color_deviation);
        */

        vec3 water_color = vec3(0.227, 0.325, 0.392);

        float light_strength = scene_gpu_data.light_position.a;
        float light_specular_factor = scene_gpu_data.light_color.a;

        vec3 light_dir = normalize(scene_gpu_data.light_position.xyz - in_frag_world_space_pos);

        vec3 ambient = water_color;

        float diff = max(dot(light_dir, normal), 0.0);
        vec3 diffuse = diff * water_color;

        vec3 view_dir = normalize(scene_gpu_data.viewer_position.xyz - in_frag_world_space_pos);
        view_dir.y *= scene_gpu_data.specular_displacement;

        vec3 halfway_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(normal, halfway_dir), 0.0), scene_gpu_data.water_shininess);
        vec3 specular = (spec * scene_gpu_data.light_color.rgb) * light_specular_factor;

        if (debug_render_world_space_pos) {
            pixel_color = vec4(in_frag_world_space_pos, 1.0);
        } else {
            pixel_color = vec4(ambient + ((diffuse + specular) * light_strength), 1.0);	
            //            pixel_color = vec4(water_color, 1.0);	
        }
    }
}
