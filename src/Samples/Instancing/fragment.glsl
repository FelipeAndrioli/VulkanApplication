#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_MATERIALS 50

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	float time;					// 4
	float lightIntensity;		// 8
	int extra_2;				// 12
	int extra_3;				// 16
	vec4 lightPosition;			// 32
	vec4 lightColor;			// 48 
	vec4 extra[5];				// 128
	mat4 view;					// 192
	mat4 proj;					// 256
} sceneGPUData;

layout (std140, set = 0, binding = 1) uniform ModelGPUData {
	int flip_uv_vertically;		// 4
	int extra_1;				// 8
	int extra_2;				// 12
	int extra_3;				// 16
	vec4 extra[7];				// 128
	mat4 model;					// 192
	mat4 normal;				// 256
} modelGPUData;

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec3 fragColor;
layout (location = 3) in vec3 fragTangent;
layout (location = 4) in vec2 fragTexCoord;

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

vec4 phongDiffuse(vec4 material_diffuse, vec4 material_normal) {
	vec3 lightDir	= normalize(vec3(sceneGPUData.lightPosition) - fragPos); 
	float diff		= max(dot(lightDir, vec3(material_normal)), 0.0);

	return material_diffuse * diff * (sceneGPUData.lightColor * sceneGPUData.lightIntensity);
}

void main() {
	
	material_t material		= materials[constants.material_index];

	vec4 material_diffuse	= vec4(1.0);
	vec4 material_normal	= vec4(1.0);
	vec4 material_color		= vec4(0.0);

	if (material.diffuse_texture_index == -1)
		material_diffuse = material.diffuse;	
	else
		material_diffuse = texture(texSampler[material.diffuse_texture_index], fragTexCoord);	

	if (material.normal_texture_index == -1)
		material_normal = vec4(fragNormal, 1.0);
	else
		material_normal = texture(texSampler[material.normal_texture_index], fragTexCoord);

	material_color += phongDiffuse(material_diffuse, material_normal);

	out_color = material_color;
}
