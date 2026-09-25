#version 450 

// local execution workgroup dimensions
layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (std140, set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
    vec4 sun;                   // xy -> pos; z -> radius; w -> strength
	vec4 viewer_position;
    vec4 water_color;           // w is ambient color strength
    vec4 displacement;
    vec4 local_space_camera_frustum_planes[6];
    int flags;
    int wave_count;
    int normal_wave_count;
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
    float tessellation_min_threshold;
    float tessellation_max_threshold;
    float tessellation_level_min;
    float tessellation_level_max;
    float tessellation_step;
    float reflection_strength;
    float image_width;
    float image_height;
    float fog_density;
    float fog_height_falloff;
} scene_gpu_data;

layout (set = 1, binding = 0) uniform wave_spectrum_parameters {
    mat4 extra[3];
    vec4 extra1[2];
    vec2 wind_direction;
    int dimension;
    float patch_size;
    float wind_speed;
    float amplitude;
    float small_wave_suppression_threshold;
} wave_parameters;

// output image layout, format, binding slot, and access type
layout (rgba32f, set = 1, binding = 1) uniform writeonly image2D displacement_map;
layout (rgba16f, set = 1, binding = 2) uniform writeonly image2D normal_map;

struct wave_function_result {
    vec3 position;
    vec3 normal;
};

wave_function_result wave_test(vec4 pos) {

    wave_function_result result;

    float amplitude = scene_gpu_data.sine_fbm_amplitude;
    float frequency = scene_gpu_data.sine_fbm_frequency;
    float speed = scene_gpu_data.wind_speed;
    float time = scene_gpu_data.time;

    float f = sin(pos.x * frequency * amplitude + (time * speed));

    result.position = vec3(pos.x, f, pos.z);
    result.normal = vec3(0., 1., 0.);

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
    bool domain_warping_enabled,
    bool wave_random_direction_enabled) {

    float height_sum = 0.0;
    float weight_sum = 0.0;
  
    vec2 position = pos.xz;
    vec2 displacement_accumulation = vec2(0.0);

    float amplitude = sine_fbm_amplitude;
    float frequency = sine_fbm_frequency;
    float direction_seed = 0.0;

    vec2 derivative_sum = vec2(0.0);
    float chop_sum = 0.0;

    for (int octave_index = 0; octave_index < scene_gpu_data.wave_count; octave_index++) {
      
        float angle_variation = sin(float(octave_index) * 45.321) * 0.785;
        float current_angle = wind_angle + angle_variation;
        float wave_speed = float(sqrt(9.81 / frequency) * wind_speed);

        vec2 dir = vec2(0.0);

        if (wave_random_direction_enabled) {
            dir = vec2(sin(direction_seed), cos(direction_seed));
        } else {
            dir = vec2(cos(current_angle), sin(current_angle));
        }

        float spatial_phase = float(dot(position, dir) * frequency);
        float temporal_phase = time * wave_speed * pow(float(frequency), temporal_phase_exponent);
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

            mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));

            vec2 offset = rot * (dir * sin(x) * (-base_derivative) * amplitude * drag_mult);
           
            displacement_accumulation += offset;
            position += offset;
        }

        amplitude *= sine_fbm_amplitude_multiplier;
        frequency *= sine_fbm_frequency_multiplier;

        direction_seed += 1232.399963;
    }

    float height = weight_sum > 0.0 ? ((height_sum / weight_sum) * height_multiplier) * water_depth - water_depth: 0.0;

    wave_function_result result;

    result.position = vec3(pos.x + displacement_accumulation.x, height, pos.z + displacement_accumulation.y);
    result.normal = normalize(vec3(-derivative_sum.x, 1.0 - chop_sum * 0.5, -derivative_sum.y));

    return result;
}

void main() {

    float time = scene_gpu_data.time;

    bool debug_render_normals               = bool(scene_gpu_data.flags & 1);
    bool circular_waves_enabled             = bool(scene_gpu_data.flags & (1 << 1));
    bool debug_render_world_space_pos       = bool(scene_gpu_data.flags & (1 << 2));
    bool domain_warping_enabled             = bool(scene_gpu_data.flags & (1 << 3));
    bool tessellation_enabled               = bool(scene_gpu_data.flags & (1 << 4));
    bool reflection_enabled                 = bool(scene_gpu_data.flags & (1 << 5));
    bool wave_random_direction_enabled      = bool(scene_gpu_data.flags & (1 << 6));

    ivec2 pixel_coord = ivec2(gl_GlobalInvocationID.xy);
    vec2 image_size = imageSize(displacement_map);

//    if (pixel_coord.x >= image_size.x || pixel_coord.y >= image_size.y) {
//        return;
//    }

    vec4 pos = vec4(pixel_coord.x, 0.0, pixel_coord.y, 1.0);

    wave_function_result result = sine_wave_fractal_brownian_motion(
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
        domain_warping_enabled,
        wave_random_direction_enabled);

    vec4 position = vec4(result.position, 1.0);
    vec4 normal = vec4(result.normal, 1.0);

    imageStore(displacement_map, pixel_coord, position);
    imageStore(normal_map, pixel_coord, normal);
}
