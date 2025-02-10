#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_MATERIALS 50

layout (location = 1) in vec3 fragNormal;
layout (location = 3) in vec2 fragTexCoord;

layout (location = 0) out vec4 out_color;

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

layout (std140, set = 0, binding = 2) uniform material_uniform {
	material_t materials[MAX_MATERIALS];
};

layout (set = 0, binding = 3) uniform sampler2D texSampler[];

layout (push_constant) uniform constant {
	int material_index;
} constants;

void main() {
	material_t material = materials[constants.material_index];

	vec4 material_color = vec4(0.0);
	vec4 material_ambient = vec4(1.0);
	vec4 material_diffuse = vec4(1.0);
	vec4 material_specular = vec4(1.0);
	vec4 material_normal = vec4(1.0);

	if (material.diffuse_texture_index == -1) {
		material_ambient = vec4(material.diffuse);
	} else {
		material_ambient = texture(texSampler[material.diffuse_texture_index], fragTexCoord);
		if (material_ambient.a < 0.1) {
			discard;
		}
	}

	if (material.diffuse_texture_index == -1) {
		material_diffuse = vec4(material.diffuse);
	} else {
		material_diffuse = texture(texSampler[material.diffuse_texture_index], fragTexCoord);
		if (material_diffuse.a < 0.1) {
			discard;
		}
	}

	if (material.normal_texture_index == -1) {
		material_normal = vec4(normalize(fragNormal), 1.0);
	} else {
		material_normal = texture(texSampler[material.normal_texture_index], fragTexCoord);
	}

	if (material.specular_texture_index == -1) {
		material_specular = vec4(material.specular);
	} else {
		material_specular = texture(texSampler[material.specular_texture_index], fragTexCoord);
	}

	material_color = material_diffuse;

	out_color = material_color;
}
