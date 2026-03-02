#version 450

#define MAX_MATERIALS 50

#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec3 in_frag_color;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_pos;
layout (location = 3) in vec2 in_tex_coord;

layout (location = 0) out vec4 pixel_color;

struct material_t {
	vec4 ambient;						// ignore w
	vec4 diffuse;		                // ignore w
	vec4 specular;						// ignore w
	vec4 transparency;					// ignore w
	vec4 emission;						// ignore w
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

	float opacity;
	float shininess;
	float shininess_strength;
	float roughness;
	float metallic;
	float sheen;
	float clearcoat_thickness;
	float clearcoat_roughness;
	float anisotropy;
	float anisotropy_rotation;
};

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
    mat4 extra;
	mat4 projection;
	mat4 view;
	vec4 light;
    vec4 extra1[3];
} scene_gpu_data;

layout (std140, set = 0, binding = 1) uniform MaterialGPUData {
    material_t materials[MAX_MATERIALS];
} material_gpu_data;

layout (set = 0, binding = 2) uniform sampler2D textures[];

layout (push_constant) uniform PushConstants {
	mat4 model;
    int material_index;
} push_constants;

void main() {

    material_t material = material_gpu_data.materials[push_constants.material_index];

    vec4 albedo = vec4(1.0, 0.0, 1.0, 1.0);

    if (material.diffuse_texture_index != -1) {
        albedo = texture(textures[material.diffuse_texture_index], in_tex_coord);
    }

    if (material.diffuse.a < 0.1) discard;

    vec3 light_pos = scene_gpu_data.light.xyz;
    float light_intensity = scene_gpu_data.light.w;

    vec3 ambient = light_intensity * albedo.rgb;

	vec3 light_dir = light_pos - in_frag_pos;

	float diff = max(dot(light_dir, in_frag_normal), 0.1);

	vec3 diffuse = diff * albedo.rgb;

	pixel_color = vec4(ambient + diffuse, 1.0);	
}
