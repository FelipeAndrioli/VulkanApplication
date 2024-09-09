#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 dir;
layout (location = 1) in vec4 color;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = color;
}
