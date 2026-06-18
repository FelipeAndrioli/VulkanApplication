#version 450

#define MAX_SINE_WAVES 32

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_texCoord;

layout (location = 0) out vec3 out_frag_color;
layout (location = 1) out vec3 out_frag_normal;
layout (location = 2) out vec3 out_frag_position;

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

void main() {

    vec4 pos = vec4(in_position, 1.0);
    vec3 normal = vec3(0.0);

    float time = scene_gpu_data.time;

    float displacement_sum = 0.0;
    float derivative_sum_x = 0.0;
    float derivative_sum_z = 0.0;

    for (int wave_index = 0; wave_index < scene_gpu_data.sine_wave_count; ++wave_index) {
        wave_data wave = wave_gpu_data.sine_wave[wave_index];

        vec2 d = normalize(wave.direction.xz);

        float amplitude = wave.amplitude;
        float frequency = wave.direction.y;
        float speed = wave.direction.w;
       
        float f = dot(d, vec2(pos.xz)) * frequency + (time * speed);
      
        float derivative_x = frequency * d.x * amplitude * cos(f);
        float derivative_z = frequency * d.y * amplitude * cos(f);

        displacement_sum += amplitude * sin(f);
        derivative_sum_x += derivative_x;
        derivative_sum_z += derivative_z;
    }

    pos.y = displacement_sum;

    vec3 binormal = vec3(1.0, derivative_sum_x, 0.0);
    vec3 tangent = vec3(0.0, derivative_sum_z, 1.0);

    normal = normalize(vec3(-derivative_sum_x, 1.0, -derivative_sum_z));

	out_frag_normal = normalize(mat3(push_constants.model) * normal);
	vec3 frag_pos = vec3(push_constants.model * pos);

	out_frag_color	= in_color;
    out_frag_position = frag_pos;

    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(frag_pos, 1.0);
}
