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
layout (location = 4) out vec3 out_frag_original_model_space_pos;

layout (std140, set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
	vec4 viewer_position;
    vec4 deep_water_color;      // w is empty
    vec4 surface_water_color;   // w is fog factor height multiplier
    vec4 water_absorption;      // w is water absorption multiplier
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
    float drag_mult,
    float wind_angle,
    float wind_speed,
    float temporal_phase_exponent,
    float height_multiplier,
    bool domain_warping_enabled) {

    float height_sum = 0.0;
    float weight_sum = 0.0;
  
    vec2 position = pos.xz;

    float amplitude = sine_fbm_amplitude;
    float frequency = sine_fbm_frequency;
    float dir_helper = 0.0;

    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));

    vec2 derivative_sum = vec2(0.0);
    float chop_sum = 0.0;

    for (int octave_index = 0; octave_index < scene_gpu_data.wave_count; octave_index++) {
      
        float angle_variation = sin(float(octave_index) * 45.321) * 0.785;
        float current_angle = wind_angle + angle_variation;
        float wave_speed = sqrt(9.81 / frequency) * wind_speed;

//        vec2 dir = vec2(cos(current_angle), sin(current_angle));
        vec2 dir = vec2(sin(dir_helper), cos(dir_helper));

        float spatial_phase = dot(position, dir) * frequency;
        float temporal_phase = time * wave_speed * pow(frequency, temporal_phase_exponent);
        float x = spatial_phase + temporal_phase;
        float f = exp(sin(x) - 1.);
        float base_derivative = f * cos(x);
    
        height_sum += f * amplitude;
        weight_sum += amplitude;

        vec2 derivative = dir * base_derivative * amplitude;
        derivative_sum += derivative;

        if (domain_warping_enabled) {
            float chop = sin(x) * (-base_derivative) * amplitude * drag_mult;
            chop_sum += length(dir) * chop;

            position += rot * (dir * sin(x) * (-base_derivative) * amplitude * drag_mult);
        }

        amplitude *= sine_fbm_amplitude_multiplier;
        frequency *= sine_fbm_frequency_multiplier;

        dir_helper += 1232.399963;
    }

    float height = weight_sum > 0.0 ? ((height_sum / weight_sum) * height_multiplier) * water_depth - water_depth : 0.0;

    wave_function_result result;

    result.position = vec3(position.x, height, position.y);
    result.normal = normalize(vec3(-derivative_sum.x, 1.0 - chop_sum * 0.5, -derivative_sum.y));

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
            scene_gpu_data.drag_mult,
            scene_gpu_data.wind_angle,
            scene_gpu_data.wind_speed,
            scene_gpu_data.temporal_phase_exponent,
            scene_gpu_data.height_multiplier,
            domain_warping_enabled);

        pos = vec4(w.position, 1.0);
        out_frag_normal = normalize(mat3(push_constants.model) * vec3(w.normal));
        out_frag_normal.z *= -1.0;
    }

    if (!gerstner_waves_enabled && !sine_waves_enabled && !sine_wave_fractal_brownian_motion_enabled) {
        out_frag_normal = normalize(mat3(push_constants.model) * in_normal);
    }
   
    out_frag_color = in_color;
    out_frag_world_space_position = vec3(push_constants.model * pos);
    out_frag_model_space_position = pos.xyz;
    out_frag_original_model_space_pos = in_position;

    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(out_frag_world_space_position, 1.0);
}
