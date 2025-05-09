#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_LIGHTS 2

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec3 fragColor;
layout (location = 3) in vec3 fragTangent;
layout (location = 4) in vec3 fragBiTangent;
layout (location = 5) in vec2 fragTexCoord;
layout (location = 6) in vec4 fragPosLightSpace[MAX_LIGHTS];

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}