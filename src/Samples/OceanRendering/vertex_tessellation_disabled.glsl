#version 450

#define MAX_SINE_WAVES 32

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec2 in_texCoord;

layout (location = 0) out vec3 out_frag_color;
layout (location = 1) out vec3 out_frag_normal;
layout (location = 2) out vec3 out_frag_position;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	mat4 projection;
	mat4 view;
    vec4 padding[3];
	vec4 light_position;
	vec4 viewer_position;
	vec4 water_color;
    int flags;
    int sine_wave_count;
    float tessellation_level_inner;
    float tessellation_level_outer;
    float time;
    float delta_t;
} scene_gpu_data;

struct wave_data {
    vec4 direction;
    float amplitude;
    float steepness;
};

layout (std140, set = 0, binding = 1) uniform WaveGPUData {
    wave_data sine_wave[MAX_SINE_WAVES];
} wave_gpu_data;

layout (push_constant) uniform PushConstants {
	mat4 model;
} push_constants;

void main() {

    vec4 pos = vec4(in_position, 1.0);
    vec3 normal = vec3(0.0);

    float time = scene_gpu_data.time;

    float displacement_sum = 0.0;
    float partial_derivative_sum_x = 0.0;
    float partial_derivative_sum_z = 0.0;
    float partial_derivative_sum = 0.0;

    for (int wave_index = 0; wave_index < scene_gpu_data.sine_wave_count; ++wave_index) {
        wave_data wave = wave_gpu_data.sine_wave[wave_index];

        float amplitude = wave.amplitude;
        float steepness = wave.steepness;
        float speed = wave.direction.w;
        float frequency = wave.direction.y;

        float f = dot(wave.direction.xz, pos.xz) * frequency + (time * speed);
        float displacement = amplitude * sin(f);
        displacement_sum += displacement;

        float partial_derivative_x = frequency * wave.direction.x * amplitude * cos(f);
        float partial_derivative_z = frequency * wave.direction.z * amplitude * cos(f);
        float partial_derivative = amplitude * frequency * cos(f);

        partial_derivative_sum_x += partial_derivative_x;
        partial_derivative_sum_z += partial_derivative_z;
    }

//    vec3 binormal = normalize(vec3(1.0, partial_derivative_sum_x, 0.0));
//    vec3 tangent = normalize(vec3(0.0, partial_derivative_sum_z, 1.0));

    vec3 binormal = normalize(vec3(1.0, partial_derivative_sum, 0.0));
    vec3 tangent = normalize(vec3(0.0, partial_derivative_sum, 1.0));

    normal = normalize(cross(tangent, binormal));

    pos.y += displacement_sum;

	out_frag_normal = normalize(mat3(push_constants.model) * normal);
	vec3 frag_pos = vec3(push_constants.model * pos);

	out_frag_color	= in_color;
    out_frag_position = frag_pos;

    gl_Position = scene_gpu_data.projection 
        * scene_gpu_data.view 
        * vec4(frag_pos, 1.0);
}

/*
float calculateWave(vec2 position, vec2 direction, float amplitude, float frequency, float speed, float time) {
    float angle = dot(direction, position) * frequency + (time * speed);
    return amplitude * sin(angle);
}

void main() {
    vUv = uv; // Pass UVs for texturing in fragment shader

    // Start with the base vertex position
    vec3 displacedPosition = position;

    // Calculate displacement for two waves and sum them
    float wave1 = calculateWave(displacedPosition.xz, uDir1, uAmp1, uFreq1, uSpeed1, uTime);
    float wave2 = calculateWave(displacedPosition.xz, uDir2, uAmp2, uFreq2, uSpeed2, uTime);
    
    // Apply combined displacement to Y (height)
    displacedPosition.y += wave1 + wave2;

    // Calculate approximate analytic normals (derivatives of sine)
    // d/dx(A * sin(kx + wt)) = A * k * cos(kx + wt)
    float waveD1 = uAmp1 * uFreq1 * cos(dot(uDir1, displacedPosition.xz) * uFreq1 + (uTime * uSpeed1));
    float waveD2 = uAmp2 * uFreq2 * cos(dot(uDir2, displacedPosition.xz) * uFreq2 + (uTime * uSpeed2));

    // Simple analytical tangent and bitangent approximations for smooth shading
    vec3 tangent = normalize(vec3(1.0, waveD1 + waveD2, 0.0));
    vec3 bitangent = normalize(vec3(0.0, waveD1 + waveD2, 1.0));
    vNormal = normalize(cross(bitangent, tangent));

    // Final transformed vertex projection
    gl_Position = projectionMatrix * modelViewMatrix * vec4(displacedPosition, 1.0);
}

*/

