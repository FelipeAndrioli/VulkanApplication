#version 450

layout (location = 0) out vec3 dir;

layout (std140, set = 0, binding = 0) readonly buffer SceneGPUData {
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
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
	vec4 color;
} push_constants;

// hardcoded cube
const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),
	vec3( 1.0,-1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),

	vec3(-1.0,-1.0,-1.0),
	vec3( 1.0,-1.0,-1.0),
	vec3( 1.0, 1.0,-1.0),
	vec3(-1.0, 1.0,-1.0)
);

const int indices[36] = int[36](
	// front
	0, 2, 1, 3, 2, 0,
	// right
	1, 6, 5, 6, 1, 2,
	// back
	7, 5, 6, 5, 7, 4,
	// left
	4, 3, 0, 3, 4, 7,
	// bottom
	4, 1, 5, 1, 4, 0,
	// top
	3, 6, 2, 6, 3, 7
);

void main() {

    const int idx = indices[gl_VertexIndex];

    const float cube_size = push_constants.color.r;

    // Note: consider moving rotation to the storage buffer since it'll also be used by the fragment shader.
    // Note: inverse rotation to align with reflection.
    mat3 rotation = mat3(cos(push_constants.color.g), 0.0, -sin(push_constants.color.g),
                         0.0, 1.0, 0.0,
                         sin(push_constants.color.g), 0.0, cos(push_constants.color.g));

    gl_Position = scene_gpu_data.projection * scene_gpu_data.view * vec4(rotation * (cube_size * pos[idx]), 1.0);

    dir = pos[idx].xyz;
}
