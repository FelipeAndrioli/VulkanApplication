#version 450

#extension GL_KHR_vulkan_glsl : enable

#define MAX_GAUSSIAN_WEIGHTS 63

layout (location = 0) out vec4 frag_color;

layout (location = 0) in vec2 uv; 

layout (set = 0, binding = 0) uniform sampler2D image;

layout (std140, set = 1, binding = 0) uniform GaussianBlurGPUData {
	vec4 weights[15];
	int total_weights;
} gaussian_gpu_data;

layout (push_constant) uniform PushConstants {
	int horizontal;
} push_constants;

void main() {

//	float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

	int mask = 1;

	mask = 1 << 1;
	bool horizontal = bool(push_constants.horizontal);

	vec2 tex_offset = 1.0 / textureSize(image, 0);
//	vec3 result = texture(image, uv).rgb * weight[0];
	vec3 result = texture(image, uv).rgb * gaussian_gpu_data.weights[0][0];

	if (horizontal) {

//		for (int i = 0; i < 5; ++i) {
		for (int i = 0; i < gaussian_gpu_data.total_weights; ++i) {
//			result += texture(image, uv + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
//			result += texture(image, uv - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
	
			int array_index = i / 4;
			int element_index = i % 4;
		
			result += texture(image, uv + vec2(tex_offset.x * i, 0.0)).rgb * gaussian_gpu_data.weights[array_index][element_index];
			result += texture(image, uv - vec2(tex_offset.x * i, 0.0)).rgb * gaussian_gpu_data.weights[array_index][element_index];
		}
		
		frag_color = vec4(result, 1.0);
	} else {
//		for (int i = 0; i < 5; ++i) {
		for (int i = 0; i < gaussian_gpu_data.total_weights; ++i) {
//			result += texture(image, uv + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
//			result += texture(image, uv - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			
			int array_index = i / 4;
			int element_index = i % 4;

			result += texture(image, uv + vec2(0.0, tex_offset.y * i)).rgb * gaussian_gpu_data.weights[array_index][element_index];
			result += texture(image, uv - vec2(0.0, tex_offset.y * i)).rgb * gaussian_gpu_data.weights[array_index][element_index];
		}

		frag_color = vec4(result, 1.0);
	}
}