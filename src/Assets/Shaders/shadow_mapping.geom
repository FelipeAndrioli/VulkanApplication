#version 450

#extension GL_ARB_geometry_shader4 : enable

#define MAX_LIGHT_SOURCES 5
#define MAX_MODELS 10

layout (triangles) in;
layout (triangle_strip, max_vertices = 3 * MAX_LIGHT_SOURCES) out; 

struct model_t {
    vec4 extra[12];
    mat4 model;
};

struct light_t {
    vec4 extra[12];
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

    model_t model = models_uniform.models[push_constants.model_index];

    for (int i = 0; i < push_constants.active_light_sources; i++) {
		light_t light = shadow_mapping_uniform.lights[i];

		gl_Layer    = i;
		gl_Position = light.light_vp * model.model * gl_in[0].gl_Position;
		EmitVertex();

		gl_Layer    = i;
		gl_Position = light.light_vp * model.model * gl_in[1].gl_Position;
		EmitVertex();

		gl_Layer    = i;
		gl_Position = light.light_vp * model.model * gl_in[2].gl_Position;
		EmitVertex();

		EndPrimitive();
    }
}