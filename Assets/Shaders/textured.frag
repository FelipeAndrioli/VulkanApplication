#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

// how to fix this hardcoded amount of textures without errors?
layout (set = 2, binding = 1) uniform sampler2D texSampler[4];

layout (push_constant) uniform constant {
	int ambient;
	int diffuse;
	int specular;
	int bump;
	int roughness;
	int metallic;
	int normal;
} TextureIndices;

// ambient -> error  
// diffuse -> error  
// specular -> error  
// bump -> error  
// roughness -> diffuse  
// metallic -> error 
// normal -> error 

// 0 -> error
// 1 -> diffuse
// 2 -> specular
// 3 -> bump (normal)

// from code
// ambient - 0
// diffuse - 1
// specular - 2
// bump - 3
// roughness - 0
// metallic - 0
// normal - 0

void main() {
	outColor = texture(texSampler[1], fragTexCoord);
}