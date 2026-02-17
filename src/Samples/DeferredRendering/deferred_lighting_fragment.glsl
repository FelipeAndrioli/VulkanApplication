#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 in_uv;

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

void main() {
	vec4 position = texture(positionTex, in_uv).rgba;
	vec4 normal = texture(normalTex, in_uv).rgba;
	vec4 albedo = texture(albedoSpecTex, in_uv).rgba;

	float specular_value = albedo.a;

	float ambient = 0.0;
	vec3 color = albedo.rgb * ambient;

	vec3 view_dir = normalize(scene_gpu_data.view_position - position).xyz;

	for (int light_index = 0; light_index < scene_gpu_data.total_lights; ++light_index) {
		light_t light = light_gpu_data.lights[light_index];

		float distance = length(light.position.xyz - position.xyz);

		/*
		float constant_attenuation = 1.0;
		float light_attenuation = 1 / (
				constant_attenuation + 
				light.linear_attenuation * light_distance_from_pixel + 
				light.quadratic_attenuation * (light_distance_from_pixel * light_distance_from_pixel));
		*/

		float light_attenuation = clamp(1.0 - distance * distance / (light.radius * light.radius), 0.0, 1.0);

		if (distance < light.radius) {
			float light_intensity = light.color.a;

			vec3 light_dir = normalize(light.position.xyz - position.xyz);

			float diff = max(dot(normal.rgb, light_dir), 0);
			vec3 diffuse = diff * albedo.rgb;

			vec3 halfway = normalize(light_dir + view_dir.xyz);
			
			float spec = pow(max(dot(normal.xyz, halfway), 0.0), 16.0);
			vec3 specular = vec3(spec * specular_value);

			color += (diffuse + specular) * light_intensity * light.color.rgb * light_attenuation;
		}
	}

	frag_color = vec4(color, 1.0);
}
