#version 450

layout (location = 0) in vec4 in_frag_world_position;
layout (location = 1) in vec4 in_light_position;
layout (location = 2) flat in int in_light_far_distance;

void main() {
	float light_distance = length(in_frag_world_position - in_light_position);

	gl_FragDepth = light_distance / in_light_far_distance;
}