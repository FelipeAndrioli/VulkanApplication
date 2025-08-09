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
	float layer_size;
};

layout (location = 0) in fragment_input fs_input;

layout (location = 0) out vec4 pixel_color;

layout (set = 0, binding = 1) uniform sampler2D in_diffuse_texture;
layout (set = 0, binding = 2) uniform sampler2D in_normal_texture;
layout (set = 0, binding = 3) uniform sampler2D in_displacement_texture;

// Note:	Since view direction vector is normalized, its z component will be in the range of 0 to 1. When the view direction
//			is largely parallel to the surface, its z component is close to 0.0 and the division returns a much larger 
//			displacement to the uv, compared to when view direction is largely perpendicular to the surface. We're adjusting
//			the size of the displacement for the uv in such a way that it offsets the texture coordinates at a larger scale 
//			when looking at a surface from an angle compared to when looking at it from the top. The technique is called
//			Parallax Mapping with Offset Limiting.

vec2 ParallaxMapping(vec2 uv, vec3 view_dir, float height_scale, bool offset_limiting) {
	float height = texture(in_displacement_texture, uv).r;
	return offset_limiting ? uv - (view_dir.xy / view_dir.z) * (height * height_scale) : uv - view_dir.xy * (height * height_scale);
}

vec2 SteepParallaxMapping(vec2 uv, vec3 view_dir, int total_layers, float layer_size, float height_scale, bool offset_limiting) {
	
//	const int total_layers	= 10;
	const float layer_depth = layer_size / total_layers;

	vec2 current_uv = uv;

	float current_layer_depth	= 0.0;
	float current_sampled_depth = texture(in_displacement_texture, current_uv).r;

	vec2 scaled_view_direction = offset_limiting ? (view_dir.xy / view_dir.z) * height_scale : view_dir.xy * height_scale;
	vec2 uv_displacement_delta = scaled_view_direction / total_layers;

	while(current_layer_depth < current_sampled_depth) {
		current_uv				-= uv_displacement_delta;
		current_sampled_depth	= texture(in_displacement_texture, current_uv).r;
		current_layer_depth		+= layer_depth;
	}

	return current_uv;
}

layout (push_constant) uniform PushConstants {
	mat4 model;
	int flags;
	int total_layers;
} push_constants;

void main() {

	bool parallax_enabled			= bool(push_constants.flags & 1);
	bool discard_oversampled_frags	= bool(push_constants.flags & (1 << 1));
	bool offset_limiting			= bool(push_constants.flags & (1 << 2));
	bool steep_parallax_mapping		= bool(push_constants.flags & (1 << 3));

	vec3 tangent_view_dir = normalize(fs_input.tangent_view_pos - fs_input.tangent_frag_pos);

	vec2 uv = fs_input.frag_uv;

	if (parallax_enabled && steep_parallax_mapping) {
		uv = SteepParallaxMapping(fs_input.frag_uv, tangent_view_dir, push_constants.total_layers, fs_input.layer_size, fs_input.height_scale, offset_limiting);
	} else if (parallax_enabled) {
		uv = ParallaxMapping(fs_input.frag_uv, tangent_view_dir, fs_input.height_scale, offset_limiting);
	}

	// Note:	Displaced texture coordinates can oversample outside the range [0, 1]. This gives unrealistic results based on
	//			the texture's wrapping mode(s). To solve this issue we can discard the fragment when it samples outside the 
	//			default texture coordinate range.
	if (discard_oversampled_frags && (uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0))
		discard;

	vec3 color = texture(in_diffuse_texture, uv).rgb;

	// Note:	Retrieve normal in range [0, 1] and transform it to range [-1, 1].
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