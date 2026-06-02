#version 450

#define MAX_SINE_WAVES 3

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
};

layout (std140, set = 0, binding = 1) uniform WaveGPUData {
    wave_data sine_wave[MAX_SINE_WAVES];
} wave_gpu_data;

void main() {

    bool debug_render_normals = bool(scene_gpu_data.flags & (1 << 1));

    if (debug_render_normals) {
        pixel_color = vec4(in_frag_normal * 5.0, 1.0);
    } else {
        vec3 water_color = scene_gpu_data.water_color.rgb;
        float water_specular_factor = scene_gpu_data.water_color.a;

        vec3 light_dir = normalize(scene_gpu_data.light_position.xyz - in_frag_pos);
       
        float diff = max(dot(light_dir, in_frag_normal), 0.1);
    
        vec3 diffuse = diff * water_color;

        vec3 view_dir = normalize(scene_gpu_data.viewer_position.xyz - in_frag_pos);
        vec3 halfway_dir = normalize(light_dir + view_dir);

        float spec = pow(max(dot(in_frag_normal, halfway_dir), 0.0), water_specular_factor);
        vec3 specular = vec3(spec);

        pixel_color = vec4(diffuse + specular, 1.0);	
    }
}
