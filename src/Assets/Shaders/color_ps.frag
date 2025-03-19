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
	float extra_s_3;
	vec4 extra[15];
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

layout (push_constant) uniform constant {
	int material_index;
	int model_index;
	int light_source_index;
	int camera_index;
} mesh_constant;

vec3 calc_directional_light(light_t light, material_t current_material, vec4 material_ambient, vec4 material_diffuse, vec4 material_specular, vec4 material_normal) {
	vec3 light_dir = normalize(-light.direction.xyz);

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

	return vec3(ambient + diffuse + specular) * light.color.rgb;
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

	return vec3(ambient + diffuse + specular) * light.color.rgb;
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
