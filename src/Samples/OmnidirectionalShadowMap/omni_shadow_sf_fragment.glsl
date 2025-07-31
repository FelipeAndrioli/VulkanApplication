#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 in_frag_world_position;
layout (location = 1) in vec4 in_light_world_position;
layout (location = 2) flat in int in_light_far_distance;

void main() {

	float light_distance = length(in_frag_world_position - in_light_world_position);	

//	convert depth from [0, x] to [0, 1] to avoid losing details since depth will be capped to 1 after fragment shader
	gl_FragDepth = light_distance / in_light_far_distance;
}