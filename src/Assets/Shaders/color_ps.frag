#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_MATERIALS 50
#define MAX_LIGHT_SOURCES 5
#define MAX_CAMERAS 10

struct FSInput {
	vec3 fragPos;
	vec3 fragNormal;
	vec3 fragColor;
	vec3 fragTangent;
	vec3 fragBiTangent;
	vec2 fragTexCoord;
	vec4 fragPosLightSpace[MAX_LIGHT_SOURCES];
	vec3 fragWorldPos;
};

layout (location = 0) in FSInput fsInput;

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
	vec4 extra;

	mat4 model;
	mat4 view_proj;	

	int type;
	int flags;
	int index;
	int pcf_samples;

	float min_bias;
	float sps_spread;
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
	light_t lights[MAX_LIGHT_SOURCES];
};

layout (set = 0, binding = 3) uniform sampler2D texSampler[];

layout (std140, set = 0, binding = 6) uniform camera_uniform {
	camera_t cameras[MAX_CAMERAS];
};

layout (set = 0, binding = 7) uniform sampler2DArray shadow_mapping;		// retrieves texels from a texture array receiving a vec2 and a layer index

layout (push_constant) uniform constant {
	int material_index;
	int model_index;
	int light_source_index;
	int camera_index;
} mesh_constant;

vec2 poisson_disk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);

float random(vec3 seed, int i) {
	vec4 seed4 = vec4(seed, i);
	float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));

	return fract(sin(dot_product) * 43758.5453);
}

float calc_shadow(vec4 normal, vec4 light_position, light_t light) {

	int mask = (1 << 1);
	
	bool shadow_map_enabled = bool(light.flags & mask);

	mask = (1 << 2);
	bool pcf_enabled = bool(light.flags & mask);
	
	mask = (1 << 3);
	bool sps_enabled = bool(light.flags & mask);

	if (!shadow_map_enabled)
		return 0.0;

	vec4 light_space_frag_pos = fsInput.fragPosLightSpace[light.index];

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

	float bias = max(sceneGPUData.max_shadow_bias * (1.0 - dot(normal.xyz, light_dir)), light.min_bias);

	float shadow = 0.0;

	// textureSize returns a vec2 of the width and height of the given sampler texture at mipmap level 0.
	// 1 divided over the texture size returns the size of a single texel that we use to offset the texture coords.
	vec2 texel_size = vec3(1.0 / textureSize(shadow_mapping, 0)).xy;

	if (pcf_enabled && sps_enabled) {	
		// PCF (percentage-closer filtering)
		// sample the surrounding texels of the depth map and average the results to produce less blocky/hard shadows
		int pcf_samples = light.pcf_samples;
		int total_pcf_samples = pcf_samples * pcf_samples;
		int pcf_begin = int(floor((pcf_samples * 0.5) * -1));
		int pcf_end = int(pcf_samples * 0.5);

		float spread = light.sps_spread;

		for (int x = pcf_begin; x < pcf_end; ++x) {
			for (int y = pcf_begin; y < pcf_end; ++y) {
				vec2 shadow_coords = proj_coords.xy + vec2(x, y) * texel_size;

				// Stratified Poisson Sampling - Randonly displace the pixel in a predefined disk-like format increasing the
				// Soft Shadow visual. If the spread is too low we have aliasing, if it's too high we get a banding effect.
				int poinsson_disk_index = int(total_pcf_samples * random(gl_FragCoord.xyy, x + y)) % total_pcf_samples;
				shadow_coords += poisson_disk[poinsson_disk_index] / spread;

				float pcf_depth = texture(shadow_mapping, vec3(shadow_coords, light.index)).r;
				shadow += current_depth - bias > pcf_depth ? 1.0 : 0.2;
			}
		}

		shadow /= total_pcf_samples;

	} else if (pcf_enabled && !sps_enabled) {
		// PCF (percentage-closer filtering)
		// sample the surrounding texels of the depth map and average the results to produce less blocky/hard shadows
		int pcf_samples = light.pcf_samples;
		int total_pcf_samples = pcf_samples * pcf_samples;
		int pcf_begin = int(floor((pcf_samples * 0.5) * -1));
		int pcf_end = int(pcf_samples * 0.5);

		for (int x = pcf_begin; x < pcf_end; ++x) {
			for (int y = pcf_begin; y < pcf_end; ++y) {
				vec2 shadow_coords = proj_coords.xy + vec2(x, y) * texel_size;
				float pcf_depth = texture(shadow_mapping, vec3(shadow_coords, light.index)).r;
				shadow += current_depth - bias > pcf_depth ? 1.0 : 0.2;
			}
		}

		shadow /= total_pcf_samples;

	} else if (sps_enabled && !pcf_enabled) {
		float spread = light.sps_spread;

		for (int i = 0; i < light.pcf_samples; i++) {
			int poisson_disk_index = int(light.pcf_samples * random(gl_FragCoord.xyy, i)) % light.pcf_samples;
			vec2 shadow_coords = proj_coords.xy + (poisson_disk[poisson_disk_index] / spread);
			float depth = texture(shadow_mapping, vec3(shadow_coords, light.index)).r;
			shadow += current_depth - bias > depth ? 1.0 : 0.2;
		}

		shadow /= 4;
	} else {
		vec2 shadow_coords = proj_coords.xy;
		float depth = texture(shadow_mapping, vec3(shadow_coords, light.index)).r;
		shadow += current_depth - bias > depth ? 1.0 : 0.2;
	}

	if (proj_coords.z > 1.0)
		shadow = 0.0;

	return shadow;
}

vec3 calc_directional_light(light_t light, material_t current_material, vec4 material_ambient, vec4 material_diffuse, vec4 material_specular, vec4 material_normal) {
	vec3 light_dir = normalize(light.direction.xyz);

	float diff = max(dot(light_dir, vec3(material_normal)), 0.0);

	vec3 ambient = material_ambient.rgb * vec3(light.ambient);
	vec3 diffuse = material_diffuse.rgb * diff * vec3(light.diffuse);

	vec3 view_dir = normalize(vec3(cameras[mesh_constant.camera_index].position) - fsInput.fragPos);

	//Phong specular model
	//vec3 reflect_dir = reflect(-light_dir, vec3(material_normal));
	//float spec = pow(max(dot(view_dir, reflect_dir), 0.0), max(1.0, current_material.shininess));
	
	//Blinn-Phong specular model
	vec3 halfway_dir = normalize(light_dir + view_dir);
	float spec = pow(max(dot(material_normal.xyz, halfway_dir), 0.0), 16.0);

	vec3 specular = material_specular.rgb * spec * vec3(light.specular);
	float shadow = calc_shadow(material_normal, light.direction, light);

	return vec3(ambient + (1.0 - shadow) * (diffuse + specular)) * light.color.rgb;
}

vec3 calc_point_light(light_t light, material_t current_material, vec4 material_ambient, vec4 material_diffuse, vec4 material_specular, vec4 material_normal) {
	float d = length(fsInput.fragPos - light.position.xyz);
	float constant_attenuation = 1.0;
	float light_attenuation = 1 / (constant_attenuation + light.linear_attenuation * d + light.quadratic_attenuation * (d * d));

	vec3 light_dir = normalize(light.position.xyz - fsInput.fragPos);

	float diff = max(dot(light_dir, vec3(material_normal)), 0.0);

	vec3 ambient = material_ambient.rgb * vec3(light.ambient) * light_attenuation;
	vec3 diffuse = material_diffuse.rgb * diff * vec3(light.diffuse) * light_attenuation;

	vec3 view_dir = normalize(vec3(cameras[mesh_constant.camera_index].position) - fsInput.fragPos);

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
	vec3 light_dir = normalize(light.position.xyz - fsInput.fragPos);

	float theta = dot(light_dir, normalize(-light.direction.xyz));
	float epsilon = light.cut_off_angle - light.outer_cut_off_angle;
	float intensity = clamp((theta - light.outer_cut_off_angle) / epsilon, 0.0, 1.0);

	float d = length(fsInput.fragPos - light.position.xyz);
	float constant_attenuation = 1.0;
	float light_attenuation = 1 / (constant_attenuation + light.linear_attenuation * d + light.quadratic_attenuation * (d * d));

	float diff = max(dot(light_dir, vec3(material_normal)), 0.0);

	vec3 ambient = material_ambient.rgb * vec3(light.ambient) * light_attenuation;
	vec3 diffuse = material_diffuse.rgb * diff * vec3(light.diffuse) * light_attenuation;

	vec3 view_dir = normalize(vec3(cameras[mesh_constant.camera_index].position) - fsInput.fragPos);

	// Phong specular model
	//vec3 reflect_dir = reflect(-light_dir, vec3(material_normal));
	//float spec = pow(max(dot(view_dir, reflect_dir), 0.0), max(1.0, current_material.shininess));

	// Blinn-Phong specular model
	vec3 halfway_dir = normalize(light_dir + view_dir);
	float spec = pow(max(dot(material_normal.xyz, halfway_dir), 0.0), 16.0);

	vec3 specular = material_specular.rgb * spec * vec3(light.specular) * light_attenuation;

	float shadow = calc_shadow(material_normal, light.position, light);

	vec3 result = vec3(ambient + (1.0 - shadow) * (diffuse + specular)) * light.color.rgb;

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
		material_ambient = texture(texSampler[current_material.diffuse_texture_index], fsInput.fragTexCoord);
		if (material_ambient.a < 0.1) {
			discard;
		}
	}

	if (current_material.diffuse_texture_index == -1) {
		material_diffuse = vec4(current_material.diffuse);
	} else {
		material_diffuse = texture(texSampler[current_material.diffuse_texture_index], fsInput.fragTexCoord);
		if (material_diffuse.a < 0.1) {
			discard;
		}
	}

	if (current_material.normal_texture_index == -1 || sceneGPUData.render_normal_map == 0) {
		material_normal = vec4(fsInput.fragNormal, 1.0);
	} else {
		// normal mapping
		// retrieve normal map in range [0, 1] and transform it to range [-1, 1]
		material_normal = normalize(texture(texSampler[current_material.normal_texture_index], fsInput.fragTexCoord) * 2.0 - 1.0);

		// create TBN matrix from tangent bitangent and normal
		mat3 tbn = mat3(fsInput.fragTangent, fsInput.fragBiTangent, fsInput.fragNormal);

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
		material_specular = texture(texSampler[current_material.specular_texture_index], fsInput.fragTexCoord);
	}


	for (int i = 0; i < sceneGPUData.total_lights; i++) {
		light_t light = lights[i];

		int active_mask = 1 << 4;
		bool is_light_active = bool(light.flags & active_mask);

		if (!is_light_active)
			continue;

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
