#version 450

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_MATERIALS 26
#define MAX_LIGHTS 5

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec2 fragTexCoord;
layout (location = 3) in vec3 fragPos;

// totals.x = total lights

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

struct light_t {
	vec4 position;
	vec4 color;		// w -> light intensity
	vec4 extra[9];

	mat4 model;

	int type;

	float scale;
	float ambient_strength;
	float specular_strength;
};

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int total_lights;
	float time;
	float extra_s_2;
	float extra_s_3;
	vec4 camera_position;
	vec4 extra[6];
	mat4 view;
	mat4 proj;
} sceneGPUData;

layout (std140, set = 0, binding = 1) uniform material_uniform {
	material_t materials[MAX_MATERIALS];
};

layout (set = 0, binding = 2) uniform light_uniform {
	light_t lights[MAX_LIGHTS];
};

layout (set = 0, binding = 3) uniform sampler2D texSampler[];

layout (push_constant) uniform constant {
	int material_index;
} mesh_constant;

void main() {
	material_t current_material = materials[mesh_constant.material_index];

	vec4 material_color = vec4(1.0);
	vec4 material_normal = vec4(1.0);

	if (current_material.diffuse_texture_index == -1) {
		material_color = vec4(current_material.diffuse.xyz, 1.0);
	} else {
		material_color = texture(texSampler[current_material.diffuse_texture_index], fragTexCoord);
	}

	if (current_material.normal_texture_index == -1) {
		material_normal = vec4(normalize(fragNormal), 1.0);
	} else {
		material_normal = texture(texSampler[current_material.normal_texture_index], fragTexCoord);
	}

	for (int i = 0; i < sceneGPUData.total_lights; i++) {
		light_t light = lights[i];

		float light_intensity = light.color.a;

		vec3 light_dir = normalize(light.position.xyz - fragPos);

		float diff = max(dot(light_dir, vec3(material_normal)), 0.0);

		vec3 ambient = vec3(light.ambient_strength);
		vec3 diffuse = diff * light.color.rgb;

		vec3 view_dir = normalize(vec3(sceneGPUData.camera_position) - fragPos);
		vec3 reflect_dir = reflect(-light_dir, vec3(material_normal));

		float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);

		vec3 specular = spec * light.specular_strength * vec3(light.color);

		material_color *= vec4(ambient + diffuse + specular, 1.0) * light_intensity;
	}

	out_color = material_color;
}
