#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 in_frag_world_position;
layout (location = 1) in vec4 in_light_position; 
layout (location = 2) in vec3 in_frag_normal;
layout (location = 3) flat in int in_light_far_distance;

layout (set = 0, binding = 2) uniform samplerCube shadow_map_sampler;

layout (location = 0) out vec4 frag_color;

void main() {
	vec3 material_color = vec3(1.0);
	vec3 material_diffuse = vec3(1.0);

	vec3 light_direction = vec3(in_frag_world_position - in_light_position);

	float ambi = 0.1;
	float diff = max(dot(normalize(-light_direction), in_frag_normal), 0.0);

	vec3 ambient = material_color * ambi;
	vec3 diffuse = material_color * diff;

	float closest_depth = texture(shadow_map_sampler, light_direction).r;
	closest_depth *= in_light_far_distance;

	float current_depth = length(light_direction);
	float bias = 0.05;
	float shadow = current_depth - bias > closest_depth ? 1.0 : 0.0;

	frag_color = vec4(ambient + ((1.0 - shadow) * diffuse), 1.0);	
}
