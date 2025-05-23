#version 450 core

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 dir;
layout (location = 0) out vec4 outColor;

//layout (set = 1, binding = 3) uniform samplerCube cubeTexture;
layout (set = 0, binding = 4) uniform samplerCube cubeTexture;

void main() {
	outColor = texture(cubeTexture, dir);
}
