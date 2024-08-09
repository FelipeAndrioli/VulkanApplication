#version 450 core

layout (location = 0) in vec3 dir;
layout (location = 0) out vec4 outColor;
layout (set = 1, binding = 3) uniform samplerCube cubeTexture;

void main() {
	outColor = texture(cubeTexture, dir);
	//outColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}