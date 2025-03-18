#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_MATERIALS 50
#define MAX_LIGHTS 5

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec3 fragColor;
layout (location = 3) in vec3 fragTangent;
layout (location = 4) in vec3 fragBiTangent;
layout (location = 5) in vec2 fragTexCoord;

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

/* light type

Undefined = -1,
Directional = 0,
PointLight = 1,
SpotLight = 2
*/

struct light_t {
	vec4 position;
	vec4 direction;
	vec4 color;
	vec4 extra[6];
	
	mat4 model;

	int type;
	int extra_0;
	
	float outer_cut_off_angle;
	float cut_off_angle;
	float raw_cut_off_angle;
	float raw_outer_cut_off_angle;
	float linear_attenuation;
	float quadratic_attenuation;
	float scale;
	float ambient;
	float diffuse;
	float specular;
};

layout (std140, set = 0, binding = 1) uniform material_uniform {
	material_t materials[MAX_MATERIALS];
};

layout (set = 0, binding = 2) uniform light_uniform {
	light_t lights[MAX_LIGHTS];
};

layout (set = 0, binding = 3) uniform sampler2D texSampler[];

layout (push_constant) uniform constant {
	int material_index;
	int model_index;
	int light_source_index;
} mesh_constant;

float linearize_depth(float depth, float near, float far) {
	float z = depth * 2.0 - 1.0; // back to NDC
	return (2.0 * near * far) / (far + near - z * (far - near));
}

vec4 render_depth() {
	float near = 0.1;
	float far = 200.0;
	float depth = linearize_depth(gl_FragCoord.z, near, far) / far;
	return vec4(vec3(depth), 1.0);
}

void main() {
	out_color = render_depth();
}
