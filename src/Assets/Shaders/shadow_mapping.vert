#version 450

#define MAX_MODELS 10
#define MAX_LIGHT_SOURCES 5

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec2 inTexCoord;

struct model_t {
	vec4 extra[12];
	mat4 model;
};

struct light_t {
	int extra_0;
	int extra_1;
	int extra_2;
	int index;
    vec4 extra[11];
    mat4 light_vp;
};

layout (std140, set = 0, binding = 0) uniform ShadowMappingGPUData {
	light_t lights[MAX_LIGHT_SOURCES];
} shadow_mapping_uniform;

layout (std140, set = 0, binding = 1) uniform ModelGPUData {
	model_t models[MAX_MODELS];
} models_uniform;

layout (push_constant) uniform PushConstants {
	int model_index;
	int active_light_sources;
} push_constants;

void main() {
	gl_Position = vec4(inPosition, 1.0);
}