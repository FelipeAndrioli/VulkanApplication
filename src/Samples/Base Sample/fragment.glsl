#version 450

layout (location = 0) in vec3 in_frag_color;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_pos;
layout (location = 3) in vec3 in_light_pos;

layout (location = 0) out vec4 pixel_color;

void main() {

	vec3 light_dir = in_light_pos - in_frag_pos;

	float diff = max(dot(light_dir, in_frag_normal), 0.1);

	vec3 diffuse = diff * in_frag_color;

	pixel_color = vec4(diffuse, 1.0);	
}