#version 420

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout (std140, set = 0, binding = 0) uniform SceneGPUData {
	float time;					// 4
	float explode;				// 8
	int extra_2;				// 12
	int extra_3;				// 16
	vec4 extra[7];				// 128
	mat4 view;					// 192
	mat4 proj;					// 256
} sceneGPUData;

layout (location = 0) in vec3 inFragPos[]; 
layout (location = 1) in vec3 inFragNormal[];
layout (location = 2) in vec3 inFragColor[];
layout (location = 3) in vec2 inFragTexCoord[];

layout (location = 0) out vec3 fragPos;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec3 fragColor;
layout (location = 3) out vec2 fragTexCoord;

// Generate a normal vertex based in the current triangle position 
vec3 getNormal() {
	vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
	vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);

	return normalize(cross(b, a));
}

// Move the triangle away/back from its original position
vec4 explode(vec4 position, vec3 normal) {
	float magnitude = 1.0;
	
	vec3 direction = normal * ((sin(sceneGPUData.time * 1.0 + 4.7) + 1.0) / 2.0) * magnitude;
	direction += normal * sceneGPUData.explode * magnitude;

	return position + vec4(direction.xy, 0.0, 0.0);
}

void main() {
	
	// Generate exploded vertices
	vec3 normal		= getNormal();

	gl_Position		= explode(gl_in[0].gl_Position, normal);
	fragTexCoord	= inFragTexCoord[0];
	fragPos			= inFragPos[0];
	fragNormal		= inFragNormal[0];
	EmitVertex();

	gl_Position		= explode(gl_in[1].gl_Position, normal);
	fragTexCoord	= inFragTexCoord[1];
	fragPos			= inFragPos[1];
	fragNormal		= inFragNormal[1];
	EmitVertex();

	gl_Position		= explode(gl_in[2].gl_Position, normal);
	fragTexCoord	= inFragTexCoord[2];
	fragPos			= inFragPos[2];
	fragNormal		= inFragNormal[2];
	EmitVertex();

	EndPrimitive();
}