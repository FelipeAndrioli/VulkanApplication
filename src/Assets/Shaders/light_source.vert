#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_MATERIALS 26
#define MAX_LIGHT_SOURCES 5
#define MAX_CAMERAS 10

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

layout (push_constant) uniform constant {
	int material_index;
	int model_index;
	int light_source_index;
	int camera_index;
} light_constant;

struct camera_t {
	vec4 extra[7];
	vec4 position;
	mat4 view;
	mat4 proj;
};

layout (std140, set = 0, binding = 6) uniform camera_uniform {
	camera_t cameras[MAX_CAMERAS];
};

layout (location = 0) out vec4 color;

// hardcoded cube
const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, -1.0 ),
	vec3(-1.0, 1.0, -1.0 ),
	vec3( 1.0, 1.0, -1.0 ),
	vec3( 1.0, -1.0, -1.0),

	vec3(-1.0,-1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3( 1.0,-1.0, 1.0)
);

const int indices[36] = int[36](
	// front
	0, 1, 2, 0, 2, 3,
	// right
	3, 2, 6, 3, 6, 7,
	// back
	4, 6, 5, 4, 7, 6,
	// left
	4, 5, 1, 4, 1, 0,
	// bottom
	3, 7, 4, 3, 4, 0,
	// top
	1, 5, 6, 1, 6, 2
);

void main() {
	int idx = indices[gl_VertexIndex];

	camera_t current_camera = cameras[light_constant.camera_index];
	light_t light = lights[light_constant.light_source_index];

	gl_Position = current_camera.proj * current_camera.view * light.model * vec4(pos[idx], 1.0);
	color = light.color;
}
