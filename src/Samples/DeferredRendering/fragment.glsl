#version 450

#extension GL_EXT_nonuniform_qualifier : enable

#define MAX_MATERIALS 50
#define MAX_LIGHTS 50

layout (location = 0) in vec3 in_frag_pos;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_color;
layout (location = 3) in vec3 in_frag_tangent;
layout (location = 4) in vec2 in_frag_uv;


layout (location = 0) out vec4 pixel_color;

struct light_t {
	vec4 position;
	vec4 direction;
	vec4 color;			// w -> light intensity
	vec4 extra;

	mat4 model;			
	mat4 viewProj;			 

	 int type;
	 int flags;					
	 int index;
	 int pcfSamples;

	 float minBias;
	 float spsSpread;
	 float outerCutOffAngle;
	 float cutOffAngle;		
	 float rawCutOffAngle;
	 float rawOuterCutOffAngle;
	 float linearAttenuation;
	 float quadraticAttenuation;
	 float scale;
	 float ambient;
	 float diffuse;
	 float specular;
};

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

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 extra[6];
	vec4 view_position;
	int total_lights;
	int extra1;
	int extra2;
	int extra3;
} scene_gpu_data;

layout (std140, set = 0, binding = 1) uniform MaterialGPUData {
	material_t materials[MAX_MATERIALS];
} material_gpu_data;

layout (set = 0, binding = 2) uniform sampler2D textures[];

layout (std140, set = 0, binding = 3) uniform LightGPUData {
	light_t lights[MAX_LIGHTS];
} light_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
	int material_index;
} push_constants;

void main() {

	vec3 color = vec3(0.0);

	material_t material = material_gpu_data.materials[push_constants.material_index];

	vec3 albedo = material.diffuse_texture_index == -1 ? vec3(in_frag_color) : texture(textures[material.diffuse_texture_index], in_frag_uv).rgb;

	for (int light_index = 0; light_index < scene_gpu_data.total_lights; ++light_index) {
		light_t light = light_gpu_data.lights[light_index];
	
		float light_intensity = light.color.a;

		vec3 light_dir = light.position.xyz - in_frag_pos;

		float diff = max(dot(light_dir, in_frag_normal), 0.1);

		vec3 ambient = vec3(0.1); 
		vec3 diffuse = vec3(diff);
	
		color += (ambient + diffuse) * light_intensity * light.color.rgb * albedo;
	}
	
	pixel_color = vec4(color, 1.0);	
//	pixel_color = vec4(in_frag_tangent, 1.0);	
}