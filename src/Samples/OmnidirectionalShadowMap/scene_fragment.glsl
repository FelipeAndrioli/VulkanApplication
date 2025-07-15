#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 in_frag_world_position;
layout (location = 1) in vec4 in_light_position; 
layout (location = 2) in vec4 in_view_position;
layout (location = 3) in vec3 in_frag_normal;
layout (location = 4) flat in int in_light_far_distance;

layout (set = 0, binding = 2) uniform samplerCube shadow_map_sampler;

layout (location = 0) out vec4 frag_color;

float raw_shadow_map(vec3 light_direction) {
	float closest_depth = texture(shadow_map_sampler, light_direction).r;
	closest_depth *= in_light_far_distance;

	float current_depth = length(light_direction);
	float bias = 0.05;
	float shadow = current_depth - bias > closest_depth ? 1.0 : 0.0;

	return shadow;
}

float pcf_shadow_map(vec3 light_direction) {
	float shadow = 0.0;
	float samples = 4.0;
	float offset = 0.1;
	float bias = 0.05;
	float current_depth = length(light_direction);

	for (float x = -offset; x < offset; x += offset / (samples * 0.5)) {
		for (float y = -offset; y < offset; y += offset / (samples * 0.5)) {
			for (float z = -offset; z < offset; z += offset / (samples * 0.5)) {
				float closest_depth = texture(shadow_map_sampler, light_direction + vec3(x, y, z)).r;
				closest_depth *= in_light_far_distance;

				if (current_depth - bias > closest_depth)
					shadow += 1.0;
			}
		}
	}

	shadow /= (samples * samples * samples);

	return shadow;
}

vec3 sample_offset_directions[20] = vec3[] (
	vec3(1, 1, 1),	vec3(1, -1, 1),		vec3(-1, -1, 1),	vec3(-1, 1, 1),
	vec3(1, 1, -1), vec3(1, -1, -1),	vec3(-1, -1, -1),	vec3(-1, 1, -1),
	vec3(1, 1, 0),	vec3(1, -1, 0),		vec3(-1, -1, 0),	vec3(-1, 1, 0),
	vec3(1, 0, 1),	vec3(-1, 0, 1),		vec3(1, 0, -1),		vec3(-1, 0, -1),
	vec3(0, 1, 1),	vec3(0, -1, 1),		vec3(0, -1, -1),	vec3(0, 1, -1)
);

float optimized_pcf_shadow_map(vec3 light_direction, vec3 view_position) {
	// Note:	When applying PCF to omnidirectional shadow maps most of the samples are redundant in that
	//			they sample close to the original direction vector it may make more sense to only sample in
	//			perpendicular directions of the sample direction vector. However as there is no (easy) way 
	//			to figure out which sub-directions are redundant this becomes difficult. One trick we can
	//			use is to take an array of offset directions that are all roughly separable e.g. each of 
	//			them points in completely different directions. This will significantly reduce the number of
	//			sub-directions that are close together.

	float shadow = 0.0;
	float bias = 0.05;
	float view_distance = length(view_position - in_frag_world_position.xyz);

	// Note:	A trick that can be applied to the disk radius is to change it based on the distance of the
	//			viewer to the fragment, making the shadows softer when far away and sharper when close by.

//	float disk_radius = 0.05;
	float disk_radius = (1.0 + (view_distance / in_light_far_distance)) / 25.0;
	float current_depth = length(light_direction);
	int samples = 20;

	// Note:	Here we add multiple offsets, scaled by some disk radius, around the original light direction
	//			vector to sample from the cubemap.

	for (int i = 0; i < samples; i++) {
		float closest_depth = texture(shadow_map_sampler, light_direction + sample_offset_directions[i] * disk_radius).r;
		closest_depth *= in_light_far_distance;

		if (current_depth - bias > closest_depth)
			shadow += 1.0;
	}

	shadow /= float(samples);

	return shadow;
}

void main() {
	vec3 material_color = vec3(1.0);
	vec3 material_diffuse = vec3(1.0);

	vec3 light_direction = vec3(in_frag_world_position - in_light_position);

	float ambi = 0.1;
	float diff = max(dot(normalize(-light_direction), in_frag_normal), 0.0);

	vec3 ambient = material_color * ambi;
	vec3 diffuse = material_color * diff;

	// TODO: Add shadow options to the scene 
//	float shadow = raw_shadow_map(light_direction);
//	float shadow = pcf_shadow_map(light_direction);
	float shadow = optimized_pcf_shadow_map(light_direction, in_view_position.xyz);

	frag_color = vec4(ambient + ((1.0 - shadow) * diffuse), 1.0);	
}
