#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 frag_color;

layout (location = 0) in vec2 uv; 

layout (set = 0, binding = 0) uniform sampler2D image;

layout (push_constant) uniform PushConstants {
	int horizontal;
} push_constants;

void main() {

	float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

	int mask = 1;

	mask = 1 << 1;
	bool horizontal = bool(push_constants.horizontal);

	vec2 tex_offset = 1.0 / textureSize(image, 0);
	vec3 result = texture(image, uv).rgb * weight[0];

	if (horizontal) {

		for (int i = 0; i < 5; ++i) {
			result += texture(image, uv + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
			result += texture(image, uv - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
		}
		
		frag_color = vec4(result, 1.0);
	} else {
		for (int i = 0; i < 5; ++i) {
			result += texture(image, uv + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
			result += texture(image, uv - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
		}

		frag_color = vec4(result, 1.0);
	}
}