#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragTexCoord;
layout (location = 3) in vec3 fragPos;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(fragPos, 1.0);	
}