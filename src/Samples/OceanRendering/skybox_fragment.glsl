#version 450

layout (set = 0, binding = 1) uniform samplerCube cube_texture;

layout (location = 0) in vec3 dir;
layout (location = 0) out vec4 color;

void main() {
    color = texture(cube_texture, dir);
}
