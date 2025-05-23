#version 450

#define MAX_MODELS 10
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

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec2 inTexCoord;

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragColor;
layout (location = 3) out vec3 fragTangent;
layout (location = 4) out vec3 fragBiTangent;
layout (location = 5) out vec2 fragTexCoord;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	int total_lights;
	float time;
	float extra_s_2;
	float extra_s_3;
	vec4 extra[15];
} sceneGPUData;

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
}

