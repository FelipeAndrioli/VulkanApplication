#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 frag_color;

layout (set = 0, binding = 0) uniform sampler2D geometry_pass_result;

layout (set = 0, binding = 1) uniform PostEffectsGPUData {
	int flags;
	float gamma;
	float hdr_exposure;
} post_effects_gpu_data;

void main() {

	bool hdr_reinhard_enabled		= bool(post_effects_gpu_data.flags);
	bool gamma_correction_enabled	= bool(post_effects_gpu_data.flags & (1 << 1));
	bool hdr_exposure_enabled		= bool(post_effects_gpu_data.flags & (1 << 2));
	bool aces_tonemapping_enabled	= bool(post_effects_gpu_data.flags & (1 << 3));

	vec3 hdr_color = texture(geometry_pass_result, uv).rgb;

	if (hdr_reinhard_enabled) {
		// reinhard tone mapping
		hdr_color = hdr_color / (hdr_color + vec3(1.0));
	} 
	
	if (hdr_exposure_enabled) {
		// exposure tone mapping
		hdr_color = vec3(1.0) - exp(-hdr_color * post_effects_gpu_data.hdr_exposure);
	}

	if (gamma_correction_enabled) {
		// gamma correction
		hdr_color = pow(hdr_color, vec3(1.0 / post_effects_gpu_data.gamma));
	}

	if (aces_tonemapping_enabled) {
		mat3 m1 = mat3(
			0.59719, 0.07600, 0.02840,
			0.35458, 0.90834, 0.13383,
			0.04823, 0.01566, 0.83777
		);

		mat3 m2 = mat3(
			1.60475, -0.10208, -0.00327,
			-0.53108,  1.10813, -0.07276,
			-0.07367, -0.00605,  1.07602
		);

		vec3 v = m1 * hdr_color;
		vec3 a = v * (v + 0.0245786) - 0.000090537;
		vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
		hdr_color = pow(clamp(m2 * (a / b), 0.0, 1.0), vec3(1.0 / 2.2));
	}

	frag_color = vec4(hdr_color, 1.0);
}