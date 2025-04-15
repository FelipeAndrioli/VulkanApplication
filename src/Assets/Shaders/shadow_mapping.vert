#version 450

#define MAX_MODELS 10
#define MAX_LIGHTS 5

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec2 inTexCoord;

struct model_t {
	vec4 extra[12];
	mat4 model;
};

layout (std140, set = 0, binding = 0) uniform ShadowMappingGPUData{
	vec4 extra[12];
	mat4 light_vp;
} shadow_mapping_uniform;

layout (std140, set = 0, binding = 1) uniform ModelGPUData {
	model_t models[MAX_MODELS];
} models_uniform;

layout (push_constant) uniform PushConstants {
	int model_index;
} push_constants;

void main() {

	model_t model = models_uniform.models[push_constants.model_index];
	
	gl_Position = shadow_mapping_uniform.light_vp * model.model * vec4(inPosition, 1.0);
}