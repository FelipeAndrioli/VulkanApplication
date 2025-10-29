#version 450

#define MAX_MATERIALS 50
#define MAX_LIGHTS 30

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 0) in vec3 in_frag_color;
layout (location = 1) in vec3 in_frag_normal;
layout (location = 2) in vec3 in_frag_pos;
layout (location = 3) in vec2 in_uv;

layout (location = 0) out vec4 pixel_color;
layout (location = 1) out vec4 bloom_color;

struct light_t {
	mat4 model;
	vec4 position;
	vec4 color;
	vec4 extra[9];
	float linear;
	float quadratic;
	float extra1;
	float extra2;
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
	vec4 extra[7];
	int total_lights;
	int extra1;
	int extra2;
	float bloom_threshold;
} scene_gpu_data;

layout(std140, set = 0, binding = 1) uniform MaterialGPUData {
	material_t materials[MAX_MATERIALS];
} material_gpu_data;

layout(set = 0, binding = 2) uniform sampler2D textures[];

layout (std140, set = 0, binding = 3) uniform LightGPUData {
	light_t lights[MAX_LIGHTS];
} light_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
	int material_index;
} push_constants;

void main() {

	material_t material = material_gpu_data.materials[push_constants.material_index];

	vec4 material_diffuse = vec4(1.0, 0.0, 1.0, 1.0);

	if (material.diffuse_texture_index != -1) {
		material_diffuse = texture(textures[material.diffuse_texture_index], in_uv);
	}

	if (material_diffuse.a < 0.1)
		discard;

	pixel_color = vec4(0.0);

	for (int light_index = 0; light_index < scene_gpu_data.total_lights; ++light_index) {
		light_t light = light_gpu_data.lights[light_index];

		vec3 light_dir = light.position.xyz - in_frag_pos;

		float light_distance	= length(light_dir);
		float constant			= 1.0;
		float linear			= light.linear * light_distance;
		float quadratic			= light.quadratic * (light_distance * light_distance);
		float light_attenuation = 1 / (constant + linear + quadratic);

		float diff		= max(dot(light_dir, in_frag_normal), 0.0);
		vec3 diffuse	= (diff * material_diffuse.rgb) * light.color.a * light.color.rgb * light_attenuation;
		
		pixel_color += vec4(diffuse, 1.0);	
	}

	// Note:	Calculate the brightness by first transforming it to grayscale, taking the dot product of both 
	//			vectors we effectively multiply each individual component of both vectors and add the results 
	//			together.
	float brightness = dot(pixel_color.rgb, vec3(0.2126, 0.7152, 0.0722));

	if (brightness > scene_gpu_data.bloom_threshold)
		bloom_color = vec4(pixel_color.rgb, 1.0);
	else
		bloom_color = vec4(vec3(0.0), 1.0);
}