#version 450

#define MAX_MODELS 10
#define MAX_LIGHT_SOURCES 5
#define MAX_CAMERAS 10

struct model_t {
	vec4 extra[7];
	mat4 model;
	mat4 normal_matrix;
	int extra_scalar;
	int extra_scalar1;
	
	int flip_uv_vertically;
	float outline_width;
};

struct camera_t {
	vec4 extra[7];
	vec4 position;
	mat4 view;
	mat4 proj;
};

struct VSOutput {
	vec3 fragPos;
	vec3 fragNormal;
	vec3 fragColor;
	vec3 fragTangent;
	vec3 fragBiTangent;
	vec2 fragTexCoord;
	vec4 fragPosLightSpace[MAX_LIGHT_SOURCES];
	vec3 fragWorldPos;
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec2 inTexCoord;

/*
layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragColor;
layout (location = 3) out vec3 fragTangent;
layout (location = 4) out vec3 fragBiTangent;
layout (location = 5) out vec2 fragTexCoord;
layout (location = 6) out vec4 fragPosLightSpace[MAX_LIGHT_SOURCES];
layout (location = 7) out vec3 worldPos;
*/

layout (location = 0) out VSOutput vsOutput; 

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

	mat4 model;
	mat4 view_proj;	

	int type;
	int flags;
	int index;
	int pcf_samples;
	int extra_0;
	int extra_1;
	int extra_2;

	float far_plane;
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

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int total_lights;
	float time;
	float extra_s_2;
	float extra_s_3;
	vec4 extra[15];
} sceneGPUData;

layout (set = 0, binding = 2) uniform light_uniform {
	light_t lights[MAX_LIGHT_SOURCES];
};

layout (std140, set = 0, binding = 5) uniform model_uniform {
	model_t models[MAX_MODELS];
};

layout (std140, set = 0, binding = 6) uniform camera_uniform {
	camera_t cameras[MAX_CAMERAS];
};

layout (push_constant) uniform constant {
	int material_index;
	int model_index;
	int light_source_index;
	int camera_index;
} mesh_constant;

void main() {
	model_t current_model = models[mesh_constant.model_index];
	camera_t current_camera = cameras[mesh_constant.camera_index];

	gl_Position = current_camera.proj * current_camera.view * current_model.model * vec4(inPosition, 1.0);
	vsOutput.fragColor = inColor;

	if (current_model.flip_uv_vertically == 1) {
		vsOutput.fragTexCoord = vec2(inTexCoord.x, inTexCoord.y * -1);
	} else {
		vsOutput.fragTexCoord = inTexCoord;
	}

	vsOutput.fragPos		= vec3(current_model.model * vec4(inPosition, 1.0));
	vsOutput.fragTangent	= normalize(vec3(current_model.normal_matrix * vec4(inTangent, 0.0)));
	vsOutput.fragNormal		= normalize(vec3(current_model.normal_matrix * vec4(inNormal, 0.0)));
	vsOutput.fragTangent	= normalize(vsOutput.fragTangent - vsOutput.fragNormal * dot(vsOutput.fragNormal, vsOutput.fragTangent));
	vsOutput.fragBiTangent	= cross(vsOutput.fragTangent, vsOutput.fragNormal);	
	vsOutput.fragWorldPos	= inPosition;

	for (int i = 0; i < sceneGPUData.total_lights; i++) {
		vsOutput.fragPosLightSpace[i] = lights[i].view_proj * vec4(vsOutput.fragPos, 1.0);
	}

	if (dot(cross(vsOutput.fragNormal, vsOutput.fragTangent), vsOutput.fragBiTangent) < 0.0)
		vsOutput.fragTangent = vsOutput.fragTangent * -1.0;
}

/*
	TODO: Create separated vertex shader for pipelines that doesn't require so much resoruces
*/

