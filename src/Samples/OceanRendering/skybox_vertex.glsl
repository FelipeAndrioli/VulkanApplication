#version 450

#define MAX_WAVES 32 

layout (std140, set = 0, binding = 0) readonly buffer SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 light_position;        // w is light strength
	vec4 light_color;           // w is light specular
	vec4 viewer_position;
    vec4 water_color;           // w is empty
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
    float tessellation_min_threshold;
    float tessellation_max_threshold;
    float tessellation_level_min;
    float tessellation_level_max;
    float tessellation_step;
    float reflection_strength;
} scene_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
	vec4 color;
} push_constants;

layout (location = 0) out vec3 dir;

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

    const float cube_size = push_constants.color.r;
    const int idx = indices[gl_VertexIndex];

    gl_Position = scene_gpu_data.projection * scene_gpu_data.view * vec4(cube_size * pos[idx], 1.0);

    dir = pos[idx].xyz;
}
