#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	float time;					// 4
	float explode;				// 8
	float magnitude;				// 12
	int extra_3;				// 16
	vec4 extra[7];				// 128
	mat4 view;					// 192
	mat4 proj;					// 256
} sceneGPUData;

layout (location = 1) in vec3 inFragNormal[];

void generateLine(int index) {
	gl_Position = sceneGPUData.proj * gl_in[index].gl_Position;
	EmitVertex();

	gl_Position = sceneGPUData.proj * (gl_in[index].gl_Position + vec4(inFragNormal[index], 0.0) * sceneGPUData.magnitude);
	EmitVertex();

	EndPrimitive();

}

void main() {
	generateLine(0);
	generateLine(1);
	generateLine(2);
}