#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

struct material_t {
	vec4 ambient;		// ignore w
	vec4 diffuse;		// ignore w
	vec4 specular;		// ignore w
	vec4 transmittance;	// ignore w
	vec4 emission;		// ignore w
	vec4 extra[6];

	int pad2;
	int illum;

	int ambient_texture_index;
	int diffuse_texture_index;
	int specular_texture_index;
	int bump_texture_index;
	int roughness_texture_index;
	int metallic_texture_index;
	int normal_texture_index;
	
	int extra_scalar;

	float shininess;
	float ior;
	float dissolve;
	float roughness;
	float metallic;
	float sheen;
	float clearcoat_thickness;
	float clearcoat_roughness;
	float anisotropy;
	float anisotropy_rotation;
	//float pad0;
};

layout (std140, set = 1, binding = 1) uniform material_uniform {
	material_t materials[26];
};

layout (set = 1, binding = 2) uniform sampler2D texSampler[];

layout (push_constant) uniform constant {
	int material_index;
} mesh_constant;

void main() {
	material_t current_material = materials[mesh_constant.material_index];
	
	if (current_material.diffuse_texture_index == -1) {
		outColor = vec4(current_material.diffuse.x, current_material.diffuse.y, current_material.diffuse.z, 1.0f);
	} else {
		outColor = texture(texSampler[current_material.diffuse_texture_index], fragTexCoord);
	}
}