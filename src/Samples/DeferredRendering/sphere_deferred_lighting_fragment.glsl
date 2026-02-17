#version 450

#extension GL_KHR_vulkan_glsl : enable

#define MAX_LIGHTS 200

layout (location = 0) in vec4 frag_pos;
layout (location = 1) in vec4 frag_normal;
layout (location = 2) in vec4 clip_space_frag_pos;

layout (location = 0) out vec4 frag_color;

struct light_t {
	vec4 position;
	vec4 direction;
	vec4 color;			// w -> light intensity

	mat4 model;			
	mat4 view_proj;			 

	int type;
	int flags;					
	int index;
	int pcf_samples;
	int extra0;
	int extra1;
	int extra2;

	float min_bias;
	float sps_spread;
	float outer_cut_off_angle;
	float cut_off_angle;		
	float raw_cut_off_angle;
	float raw_outer_cut_off_angle;
	float linear_attenuation;
	float quadratic_attenuation;
	float scale;
	float ambient;
	float diffuse;
	float specular;
	float radius;
};

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 extra[6];
	vec4 view_position;
	int total_lights;
	int extra1;
	int extra2;
	int extra3;
} scene_gpu_data;

layout (std140, set = 0, binding = 1) readonly buffer LightGPUData {
	light_t lights[];
} light_gpu_data;

layout (set = 0, binding = 2) uniform sampler2D positionTex;
layout (set = 0, binding = 3) uniform sampler2D normalTex;
layout (set = 0, binding = 4) uniform sampler2D albedoSpecTex;

layout (push_constant) uniform PushConstants {
	mat4 model;
	int light_index;
} push_constants;

#define PI 3.141592653589793

void main() {

	vec3 sphere_normal = normalize(frag_normal.xyz);

	vec3 clip_space = clip_space_frag_pos.xyz / clip_space_frag_pos.w;
	vec2 sphere_uv = vec2((clip_space.xy + 1) * 0.5);

	vec4 position = texture(positionTex, sphere_uv).rgba;
	vec4 normal = texture(normalTex, sphere_uv).rgba;
	vec4 albedo = texture(albedoSpecTex, sphere_uv).rgba;

	float specular_value = albedo.a;

	float ambient = 0.0;
	vec3 color = albedo.rgb * ambient;

	vec3 view_dir = normalize(scene_gpu_data.view_position - position).xyz;

	light_t light = light_gpu_data.lights[push_constants.light_index];

	float light_distance_from_pixel = length(position.xyz - light.position.xyz);

	/*
	float constant_attenuation = 1.0;

	float light_attenuation = 1 / (
			constant_attenuation + 
			light.linear_attenuation * light_distance_from_pixel + 
			light.quadratic_attenuation * (light_distance_from_pixel * light_distance_from_pixel));
	*/

	float light_attenuation = clamp(1.0 - light_distance_from_pixel * light_distance_from_pixel / (light.radius * light.radius), 0.0, 1.0);
	float light_intensity = light.color.a;

	vec3 light_dir = normalize(light.position.xyz - position.xyz);

	float diff = max(dot(normal.rgb, light_dir), 0);
	vec3 diffuse = diff * albedo.rgb;

	vec3 halfway = normalize(light_dir + view_dir.xyz);
	
	float spec = pow(max(dot(normal.xyz, halfway), 0.0), 16.0);
	vec3 specular = vec3(spec * specular_value);

	color += (diffuse + specular) * light_intensity * light.color.rgb * light_attenuation;

	frag_color = vec4(color, 1.0);
}
