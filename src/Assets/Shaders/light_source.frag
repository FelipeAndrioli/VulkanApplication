#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 light_dir;
layout (location = 1) in vec4 color;
layout (location = 2) in vec4 frag_pos;
layout (location = 3) flat in int light_type;

layout (location = 0) out vec4 out_color;

void main() {
    if (light_type == 2) {
        float diff = max(dot(vec3(frag_pos), vec3(light_dir)), 0.02);
        vec4 diffuse = color * diff;
        out_color = diffuse;
    } else {
        out_color = color;
    }
}
