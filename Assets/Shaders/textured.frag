#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

// how to fix this hardcoded amount of textures without errors?
layout (set = 2, binding = 1) uniform sampler2D texSampler[];

layout (push_constant) uniform constant {
	int ambient;
	int diffuse;
	int specular;
	int bump;
	int roughness;
	int metallic;
	int normal;
} TextureIndices;

void main() {
	outColor = texture(texSampler[TextureIndices.diffuse], fragTexCoord);
}