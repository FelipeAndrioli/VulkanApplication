#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_LIGHT_SOURCES 5

struct FSInput {
	vec3 fragPos;
	vec3 fragNormal;
	vec3 fragColor;
	vec3 fragTangent;
	vec3 fragBiTangent;
	vec2 fragTexCoord;
	vec4 fragPosLightSpace[MAX_LIGHT_SOURCES];
	vec3 fragWorldPos;
};

layout (location = 0) in FSInput fsInput;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}