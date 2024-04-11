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
		static std::vector<Mesh> GenerateGridPlaneMesh(glm::vec3 pos, float cellSize, int width, int height);
	};
}
