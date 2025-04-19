#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_MATERIALS 50
#define MAX_LIGHTS 5
#define MAX_CAMERAS 10

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 fragNormal;
layout (location = 2) in vec3 fragColor;
layout (location = 3) in vec3 fragTangent;
layout (location = 4) in vec3 fragBiTangent;
layout (location = 5) in vec2 fragTexCoord;
layout (location = 6) in vec4 fragPosLightSpace;

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
	vec4 extra[2];

	mat4 model;
	mat4 viewProj;	

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

struct camera_t {
	vec4 extra[7];
	vec4 position;
	mat4 view;
	mat4 proj;
};

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int total_lights;
	int render_normal_map;
	float time;
	float min_shadow_bias;
	float max_shadow_bias;
	float extra_s_1;
	float extra_s_2;
	float extra_s_3;
	vec4 extra[14];
} sceneGPUData;

layout (std140, set = 0, binding = 1) uniform material_uniform {
	material_t materials[MAX_MATERIALS];
};

layout (set = 0, binding = 2) uniform light_uniform {
	light_t lights[MAX_LIGHTS];
};

layout (set = 0, binding = 3) uniform sampler2D texSampler[];

layout (std140, set = 0, binding = 6) uniform camera_uniform {
	camera_t cameras[MAX_CAMERAS];
};

layout (set = 0, binding = 7) uniform sampler2D shadow_mapping;	// sampler2DShadow???

layout (push_constant) uniform constant {
	int material_index;
	int model_index;
	int light_source_index;
	int camera_index;
} mesh_constant;

float calc_shadow(vec4 light_space_frag_pos, vec4 normal, vec4 light_position) {

	vec3 light_dir = normalize(light_position.xyz - light_space_frag_pos.xyz);

	// the pipeline transition between vertex to fragment shader automatically perform the perspective divide in gl_Position,
	// in order to compare it with the light space frag position we must perform the perspective divide on it. It has no
	// impact when using orthographic projection, but it's essential to linearize perspective projection.
	// [-1, 1]
	vec3 proj_coords = light_space_frag_pos.xyz / light_space_frag_pos.w;

	// [0, 1]
	// Differently than OpenGL, Vulkan has its depth coordinate range already in [0, 1], therefore the conversion can be skipped.
	proj_coords = vec3(proj_coords.xy * 0.5 + 0.5, proj_coords.z);

	float current_depth = proj_coords.z;

	float bias = max(sceneGPUData.max_shadow_bias * (1.0 - dot(normal.xyz, light_dir)), sceneGPUData.min_shadow_bias);

	float shadow = 0.0;

	// textureSize returns a vec2 of the width and height of the given sampler texture at mipmap level 0.
	// 1 divided over the texture size returns the size of a single texel that we use to offset the texture coords.
	vec2 texel_size = 1.0 / textureSize(shadow_mapping, 0);
	
	// PCF (percentage-closer filtering)
	// sample the surrounding texels of the depth map and average the results to produce less blocky/hard shadows
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			float pcf_depth = texture(shadow_mapping, proj_coords.xy + vec2(x, y) * texel_size).r;
			shadow += current_depth - bias > pcf_depth ? 1.0 : 0.5;
		}
	}

	shadow /= 9.0;

	if (proj_coords.z > 1.0)
		shadow = 0.0;

	return shadow;
}

vec3 calc_directional_light(light_t light, material_t current_material, vec4 material_ambient, vec4 material_diffuse, vec4 material_specular, vec4 material_normal) {
	vec3 light_dir = normalize(light.direction.xyz);

	float diff = max(dot(light_dir, vec3(material_normal)), 0.0);

	vec3 ambient = material_ambient.rgb * vec3(light.ambient);
	vec3 diffuse = material_diffuse.rgb * diff * vec3(light.diffuse);

	vec3 view_dir = normalize(vec3(cameras[mesh_constant.camera_index].position) - fragPos);

	//Phong specular model
	//vec3 reflect_dir = reflect(-light_dir, vec3(material_normal));
	//float spec = pow(max(dot(view_dir, reflect_dir), 0.0), max(1.0, current_material.shininess));
	
	//Blinn-Phong specular model
	vec3 halfway_dir = normalize(light_dir + view_dir);
	float spec = pow(max(dot(material_normal.xyz, halfway_dir), 0.0), 16.0);

	vec3 specular = material_specular.rgb * spec * vec3(light.specular);

	float shadow = calc_shadow(fragPosLightSpace, material_normal, light.direction);

	return vec3(ambient + (1.0 - shadow) * (diffuse + specular)) * light.color.rgb;
}

vec3 calc_point_light(light_t light, material_t current_material, vec4 material_ambient, vec4 material_diffuse, vec4 material_specular, vec4 material_normal) {
	float d = length(fragPos - light.position.xyz);
	float constant_attenuation = 1.0;
	float light_attenuation = 1 / (constant_attenuation + light.linear_attenuation * d + light.quadratic_attenuation * (d * d));

	vec3 light_dir = normalize(light.position.xyz - fragPos);

	float diff = max(dot(light_dir, vec3(material_normal)), 0.0);

	vec3 ambient = material_ambient.rgb * vec3(light.ambient) * light_attenuation;
	vec3 diffuse = material_diffuse.rgb * diff * vec3(light.diffuse) * light_attenuation;

	vec3 view_dir = normalize(vec3(cameras[mesh_constant.camera_index].position) - fragPos);

	// Phong specular model
	//vec3 reflect_dir = reflect(-light_dir, vec3(material_normal));
	//float spec = pow(max(dot(view_dir, reflect_dir), 0.0), max(1.0, current_material.shininess));

	// Blinn-Phong specular model
	vec3 halfway_dir = normalize(light_dir + view_dir);
	float spec = pow(max(dot(material_normal.xyz, halfway_dir), 0.0), 16.0);

	vec3 specular = material_specular.rgb * spec * vec3(light.specular) * light_attenuation;

	float shadow = 0.0;

	return vec3(ambient + (1.0 - shadow) * (diffuse + specular)) * light.color.rgb;
}

vec3 calc_spot_light(light_t light, material_t current_material, vec4 material_ambient, vec4 material_diffuse, vec4 material_specular, vec4 material_normal) {
	vec3 light_dir = normalize(light.position.xyz - fragPos);

	float theta = dot(light_dir, normalize(-light.direction.xyz));
	float epsilon = light.cut_off_angle - light.outer_cut_off_angle;
	float intensity = clamp((theta - light.outer_cut_off_angle) / epsilon, 0.0, 1.0);
	
	vec3 result = calc_point_light(light, current_material, material_ambient, material_diffuse, material_specular, material_normal);

	return result * intensity;
}

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
	material_t current_material = materials[mesh_constant.material_index];

	vec4 material_color = vec4(0.0);
	vec4 material_ambient = vec4(1.0);
	vec4 material_diffuse = vec4(1.0);
	vec4 material_specular = vec4(1.0);
	vec4 material_normal = vec4(1.0);

	if (current_material.diffuse_texture_index == -1) {
		material_ambient = vec4(current_material.diffuse);
	} else {
		material_ambient = texture(texSampler[current_material.diffuse_texture_index], fragTexCoord);
		if (material_ambient.a < 0.1) {
			discard;
		}
	}

	if (current_material.diffuse_texture_index == -1) {
		material_diffuse = vec4(current_material.diffuse);
	} else {
		material_diffuse = texture(texSampler[current_material.diffuse_texture_index], fragTexCoord);
		if (material_diffuse.a < 0.1) {
			discard;
		}
	}

	if (current_material.normal_texture_index == -1 || sceneGPUData.render_normal_map == 0) {
		material_normal = vec4(fragNormal, 1.0);
	} else {
		// normal mapping
		// retrieve normal map in range [0, 1] and transform it to range [-1, 1]
		material_normal = normalize(texture(texSampler[current_material.normal_texture_index], fragTexCoord) * 2.0 - 1.0);

		// create TBN matrix from tangent bitangent and normal
		mat3 tbn = mat3(fragTangent, fragBiTangent, fragNormal);

		// multiply the normal retrieved from texture by the TBN matrix to transform the normal from tangent space to world space
		material_normal = vec4(normalize(tbn * vec3(material_normal)), 1.0);

		// When performing normal mapping the most effecient approach would be to do the opposite from what I did. Instead of 
		// multiplying the normal by the TBN matrix, the light related vectors should be multiplied by the inverse (or transpose)
		// of TBN matrix to transform them from world space to tangent space and perform all lighting calculations in that space.
		// The advantage would be to avoid matrix multiplications in fragment shader and move them all to the vertex shader, however,
		// since this shader is responsible for rendering both textured and non textured meshes, I will keep it here for now.
		// Another possible optimization to save data sending from CPU to the GPU is to load the tangent and bitangent in CPU and send 
		// it to the GPU, then the normal can be generated by doing a cross product of those vectors.
	}

	if (current_material.specular_texture_index == -1) {
		material_specular = vec4(current_material.specular);
	} else {
		material_specular = texture(texSampler[current_material.specular_texture_index], fragTexCoord);
	}


	for (int i = 0; i < sceneGPUData.total_lights; i++) {
		light_t light = lights[i];

		switch (light.type) {
			case 0:
				material_color.rgb += calc_directional_light(light, current_material, material_ambient, material_diffuse, material_specular, material_normal);
				break;
			case 1:
				material_color.rgb += calc_point_light(light, current_material, material_ambient, material_diffuse, material_specular, material_normal);
				break;
			case 2:
				material_color.rgb += calc_spot_light(light, current_material, material_ambient, material_diffuse, material_specular, material_normal);
				break;
			default:
				break;
		}
	}
	
	material_color.a = material_diffuse.a;


	out_color = vec4(material_color.rgb, 1.0);
}
