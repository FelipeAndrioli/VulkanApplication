#version 450

#extension GL_KHR_vulkan_glsl : enable

#define MAX_LIGHTS 50

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 frag_color;

struct light_t {
	vec4 position;
	vec4 direction;
	vec4 color;			// w -> light intensity
	vec4 extra;

	mat4 model;			
	mat4 viewProj;			 

	 int type;
	 int flags;					
	 int index;
	 int pcfSamples;

	 float minBias;
	 float spsSpread;
	 float outerCutOffAngle;
	 float cutOffAngle;		
	 float rawCutOffAngle;
	 float rawOuterCutOffAngle;
	 float linearAttenuation;
	 float quadraticAttenuation;
	 float scale;
	 float ambient;
	 float diffuse;
	 float specular;
};

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
	vec4 extra[6];
	vec4 view_position;
	int total_lights;
	int extra1;
	int extra2;
	int extra3;
} scene_gpu_data;

layout (std140, set = 0, binding = 1) uniform LightGPUData {
	light_t lights[MAX_LIGHTS];
} light_gpu_data;

layout (set = 0, binding = 2) uniform sampler2D positionTex;
layout (set = 0, binding = 3) uniform sampler2D normalTex;
layout (set = 0, binding = 4) uniform sampler2D albedoSpecTex;

void main() {
	vec4 position = texture(positionTex, in_uv).rgba;
	vec4 normal = texture(normalTex, in_uv).rgba;
	vec4 albedo = texture(albedoSpecTex, in_uv).rgba;

	float specular = albedo.a;

	vec3 color = vec3(0.0);

	vec3 view_dir = normalize(scene_gpu_data.view_position - position).xyz;

	for (int light_index = 0; light_index < scene_gpu_data.total_lights; ++light_index) {
		light_t light = light_gpu_data.lights[light_index];

		float light_intensity = light.color.a;

		vec3 light_dir = light.position.xyz - position.xyz;

		float diff = max(dot(light_dir, normal.rgb), 0);
		vec3 diffuse = vec3(diff);

		vec3 halfway = normalize(light_dir + view_dir.xyz);
		
		float spec = pow(max(dot(normal.xyz, halfway), 0.0), 16.0);
		vec3 specular = vec3(spec * specular);

		color += vec3((diffuse + specular) * light_intensity * light.color.rgb * albedo.rgb);
	}

	frag_color = vec4(color, 1.0);
}
