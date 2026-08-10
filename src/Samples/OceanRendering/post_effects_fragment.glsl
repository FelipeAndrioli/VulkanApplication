#version 450

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 frag_color;

layout (set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
    vec4 sun;                   // xy -> pos; z -> radius; w -> strength
	vec4 viewer_position;
    vec4 water_color;           // w is empty
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

layout (set = 0, binding = 1) uniform sampler2D skybox_texture;
layout (set = 0, binding = 2) uniform sampler2D offscreen_pass_color_result;
layout (set = 0, binding = 3) uniform sampler2D offscreen_pass_depth_result;

const float near = 0.1;
const float far = 4000.0;

float calculate_fog_factor(vec2 uv) {
    float depth = texture(offscreen_pass_depth_result, uv).r;
    float ndc_depth = depth * 2.0 - 1.0;
    float linear_depth = (2.0 * near * far) / (far + near - ndc_depth * (far - near));
    float view_distance = linear_depth * far;
    float exponential_fog_factor = exp(-scene_gpu_data.fog_density * view_distance);
    float fog_factor = clamp(exponential_fog_factor, 0., 1.);
  
    return fog_factor;
}

vec3 calculate_world_space_pos(vec2 uv, float depth) {

    vec3 ndc = vec3(uv * 2.0 - 1.0, depth * 2.0 - 1.0);
    vec4 clip_pos = vec4(ndc, 1.0);
    vec4 view_pos = inverse(scene_gpu_data.projection) * clip_pos;
    view_pos.xyz /= view_pos.w;

    vec4 world_pos = inverse(scene_gpu_data.view) * vec4(view_pos.xyz, 1.0);
    return world_pos.xyz;
}

float calculate_fog_factor(vec3 world_space_pos, float depth) {
    /*      
        Screen space exponential height fog and skybox
       
        - Fog Analytical Integral: Inigo Quilez (https://iquilezles.org/articles/fog/)
        - Skybox Horizon Fogging Technique: Industry standard approach matching Unreal Engine's Exponential Height Fog behavior.
    */
    vec3 view_dir = world_space_pos - scene_gpu_data.viewer_position.xyz;
    float view_dir_distance = length(view_dir);
    vec3 normalized_view_dir = normalize(view_dir);

    if (depth > 0.9999) {
        view_dir_distance = far;        
    }

    float fog_height_falloff = scene_gpu_data.fog_height_falloff;
    float fog_density = scene_gpu_data.fog_density;
    //    float fog_extinsion = (fog_density / fog_height_falloff) * exp(-fog_height_falloff * scene_gpu_data.viewer_position.y) * (1.0 - exp(-fog_height_falloff * normalized_view_dir.y * view_dir_distance)) / normalized_view_dir.y;
    float fog_extinsion = (fog_density / fog_height_falloff) * (exp(-fog_height_falloff * scene_gpu_data.viewer_position.y) / normalized_view_dir.y) * (1.0 - exp(-fog_height_falloff * normalized_view_dir.y * view_dir_distance));

    if (abs(normalized_view_dir.y) < 0.001) {
        fog_extinsion = fog_density * exp(-fog_height_falloff * scene_gpu_data.viewer_position.y) * view_dir_distance;
    }

    float fog_factor = clamp(exp(-fog_extinsion), 0.0, 1.0);

    return fog_factor;
}

void main() {

    float depth = texture(offscreen_pass_depth_result, uv).r;
    vec3 world_space_pos = calculate_world_space_pos(uv, depth);

    float fog_factor = calculate_fog_factor(world_space_pos, depth);

    vec3 scene_color = texture(offscreen_pass_color_result, uv).rgb;
    vec3 final_color = mix(vec3(1.0), scene_color, fog_factor);
    frag_color = vec4(final_color, 1.0);
}
