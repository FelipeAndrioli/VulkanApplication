#version 450

#define MAX_WAVES 32

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_texCoord;

layout (location = 0) out vec3 out_frag_color;
layout (location = 1) out vec3 out_frag_normal;
layout (location = 2) out vec3 out_frag_world_space_position;
layout (location = 3) out vec3 out_frag_model_space_position;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;
	vec4 light_color;   // w is specular
	vec4 viewer_position;
	vec4 deep_water_color;
	vec4 shallow_water_color;
    int flags;
    int wave_count;
    float tessellation_level_inner;
    float tessellation_level_outer;
    float time;
    float water_depth;
    float sine_fbm_amplitude;
    float sine_fbm_frequency;
    float sine_fbm_amplitude_multiplier;
    float sine_fbm_frequency_multiplier;
} scene_gpu_data;

// Note: wave direction: X and Z are directions, Y is length and W is speed.
// Note: circular wave: X and Y corresponds to the wave center X and Z.
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
    }

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

wave_function_result sine_wave_fractal_brownian_motion(
    vec4 pos, 
    float time, 
    float sine_fbm_amplitude,
    float sine_fbm_frequency,
    float sine_fbm_amplitude_multiplier,
    float sine_fbm_frequency_multiplier,
    float water_depth,
    bool domain_warping_enabled) {

    float displacement_sum = 0.0;
    float weight_sum = 0.0;
    float derivative_sum_x = 0.0;
    float derivative_sum_z = 0.0;

    wave_data placeholder_wave = wave_gpu_data.wave[0];

    float frequency = sine_fbm_frequency;
    float amplitude = sine_fbm_amplitude;
    float drag_mult = 0.38;
    float time_multiplier = 2.0;

    float wave_dir_helper = 0.0;

    vec3 position = pos.xyz;

    for (int wave_index = 0; wave_index < scene_gpu_data.wave_count; ++wave_index) {
        
        vec2 wave_dir = vec2(sin(wave_dir_helper), cos(wave_dir_helper));
        vec2 sample_pos = domain_warping_enabled ? position.xz : pos.xz;

//        float wave_phase_shift = length(sample_pos) * 0.1;
        float wave_phase_shift = dot(sample_pos, wave_dir) * 0.1;

        float f = dot(wave_dir, vec2(sample_pos)) * frequency + (time * time_multiplier + wave_phase_shift);
        float wave_height = exp(sin(f) - 1.0);
        float derivative = wave_height * cos(f);
       
        position.xz += wave_dir * derivative * amplitude * drag_mult;

        derivative_sum_x += wave_dir.x * derivative * frequency * amplitude;
        derivative_sum_z += wave_dir.y * derivative * frequency * amplitude;

        displacement_sum += wave_height * amplitude;
        weight_sum += amplitude;

        frequency *= sine_fbm_frequency_multiplier;
        amplitude *= sine_fbm_amplitude_multiplier;
        time_multiplier *= 1.04;

//        wave_dir_helper += 2.399963;
        wave_dir_helper += 7.1259;
    }

    wave_function_result result;

    result.position = vec3(position.x, pos.y + (displacement_sum / weight_sum) * water_depth - water_depth, position.z);
    result.normal = normalize(vec3(-derivative_sum_x, 1.0, -derivative_sum_z));

    return result;
}

void main() {

    vec4 pos = vec4(in_position, 1.0);

    float time = scene_gpu_data.time;

    bool sine_waves_enabled                         = bool(scene_gpu_data.flags & 1);
    bool circular_waves_enabled                     = bool(scene_gpu_data.flags & (1 << 2));
    bool gerstner_waves_enabled                     = bool(scene_gpu_data.flags & (1 << 3));
    bool sine_wave_fractal_brownian_motion_enabled  = bool(scene_gpu_data.flags & (1 << 5));
    bool domain_warping_enabled                     = bool(scene_gpu_data.flags & (1 << 6));

    if (gerstner_waves_enabled) {
        wave_function_result w = gerstner_wave(pos, time, circular_waves_enabled);
        pos = vec4(w.position, 1.0);
        out_frag_normal = normalize(mat3(push_constants.model) * vec3(w.normal));
    } 

    if (sine_waves_enabled) {
        wave_function_result w = sine_wave(pos, time, circular_waves_enabled);
        pos.y = w.displacement_sum;
        // Note:    Normal generated on fragment shader.
        out_frag_normal = normalize(mat3(push_constants.model) * vec3(w.normal));
    }

    if (sine_wave_fractal_brownian_motion_enabled) {
        wave_function_result w = sine_wave_fractal_brownian_motion(
            pos, 
            time, 
            scene_gpu_data.sine_fbm_amplitude,
            scene_gpu_data.sine_fbm_frequency,
            scene_gpu_data.sine_fbm_amplitude_multiplier,
            scene_gpu_data.sine_fbm_frequency_multiplier,
            scene_gpu_data.water_depth,
            domain_warping_enabled);

        pos = vec4(w.position, 1.0);
        out_frag_normal = normalize(mat3(push_constants.model) * vec3(w.normal));
    }

    if (!gerstner_waves_enabled && !sine_waves_enabled && !sine_wave_fractal_brownian_motion_enabled) {
        out_frag_normal = normalize(mat3(push_constants.model) * in_normal);
    }
   
    out_frag_color = in_color;
    out_frag_world_space_position = vec3(push_constants.model * pos);
    out_frag_model_space_position = pos.xyz;

    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(out_frag_world_space_position, 1.0);
}
