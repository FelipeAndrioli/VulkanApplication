#pragma once

#include <vector>
#include <glm.hpp>

namespace Assets {
	struct Mesh;

	class MeshGenerator {
	public:
		MeshGenerator() {};
		~MeshGenerator() {};

		static std::vector<Mesh> GenerateSinglePlaneMesh(glm::vec3 pos, float cellSize);
		static std::vector<Mesh> GenerateDisconnectedPlaneMesh(glm::vec3 pos, float cellSize, size_t planeSize);
		static std::vector<Mesh> GeneratePlaneMesh(glm::vec3 pos, float cellSize, size_t planeSize);
		static std::vector<Mesh> GenerateCubeMesh(glm::vec3 pos, float size);
		static std::vector<Mesh> GenerateQuadMesh(const glm::vec3 pos, const float size);
		static glm::vec3 GenerateTangentVector(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3);
		static glm::vec3 GenerateBiTangentVector(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3);
	};
}
