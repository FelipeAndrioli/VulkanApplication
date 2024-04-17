#pragma once

#include <vector>
#include <glm/glm.hpp>

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
	};
}
