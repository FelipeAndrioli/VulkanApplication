#version 450

#extension GL_KHR_vulkan_glsl : enable

struct fragment_input {
	mat3 tbn;
	vec3 light_pos;

	vec3 tangent_light_pos;
	vec3 tangent_frag_pos;
	vec3 tangent_view_pos;

	vec3 frag_pos;
	vec3 frag_normal;
	vec3 frag_color;
	vec3 frag_tangent;
	vec3 frag_bitangent;
	vec3 view_pos;
	vec2 frag_uv;

	float height_scale;
};

layout (location = 0) in fragment_input fs_input;

layout (location = 0) out vec4 pixel_color;

layout (set = 0, binding = 1) uniform sampler2D in_diffuse_texture;
layout (set = 0, binding = 2) uniform sampler2D in_normal_texture;
layout (set = 0, binding = 3) uniform sampler2D in_displacement_texture;

vec2 ParallaxMapping(vec2 uv, vec3 view_dir, float height_scale) {
	float height = texture(in_displacement_texture, uv).r;
	return uv - view_dir.xy * (height * height_scale);
}

layout (push_constant) uniform PushConstants {
	mat4 model;
	int flags;
} push_constants;

void main() {

	bool parallax_enabled = bool(push_constants.flags & 1);
	bool discard_oversampled_frags = bool(push_constants.flags & (1 << 1));

	vec3 tangent_view_dir = normalize(fs_input.tangent_view_pos - fs_input.tangent_frag_pos);

	vec2 uv = parallax_enabled ? ParallaxMapping(fs_input.frag_uv, tangent_view_dir, fs_input.height_scale) : fs_input.frag_uv;

	// Note:	Displaced texture coordinates can oversample outside the range [0, 1]. This gives unrealistic results based on
	//			the texture's wrapping mode(s). To solve this issue we can discard the fragment when it samples outside the 
	//			default texture coordinate range.
	if (discard_oversampled_frags && (uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0))
		discard;

	vec3 color = texture(in_diffuse_texture, uv).rgb;

	// retrieve normal in range [0, 1] and transform it to range [-1, 1]
	vec3 normal = normalize(texture(in_normal_texture, uv).rgb * 2.0 - 1.0);

	vec3 tangent_light_dir = normalize(fs_input.tangent_light_pos - fs_input.tangent_frag_pos);

	vec3 ambient = 0.1 * color;

	float diff = max(dot(tangent_light_dir, normal), 0.0);

	vec3 diffuse = (diff * color) * 0.7;

	// vec3 reflect_dir = reflect(-tangent_light_dir, normal);
	vec3 halfway_dir = normalize(tangent_light_dir + tangent_view_dir);

	float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);

	vec3 specular = vec3(0.5) * spec;

	pixel_color = vec4(ambient + diffuse + specular, 1.0);	
}