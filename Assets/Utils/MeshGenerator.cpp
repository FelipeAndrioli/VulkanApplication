#include "./MeshGenerator.h"
#include "../Mesh.h"

namespace Assets {
	std::vector<Mesh> MeshGenerator::GenerateSinglePlaneMesh(glm::vec3 pos, float cellSize) {
		Assets::Mesh mesh = Assets::Mesh();

		Assets::Vertex vertex[4];

		vertex[0].pos = glm::vec3(pos.x, pos.y, pos.z + cellSize);
		vertex[1].pos = glm::vec3(pos.x + cellSize, pos.y, pos.z + cellSize);
		vertex[2].pos = glm::vec3(pos.x, pos.y, pos.z);
		vertex[3].pos = glm::vec3(pos.x + cellSize, pos.y, pos.z);

		mesh.Vertices.push_back(vertex[0]);
		mesh.Vertices.push_back(vertex[1]);
		mesh.Vertices.push_back(vertex[2]);
		mesh.Vertices.push_back(vertex[3]);

		mesh.Indices.push_back(2);
		mesh.Indices.push_back(0);
		mesh.Indices.push_back(1);
		mesh.Indices.push_back(3);
		mesh.Indices.push_back(2);
		mesh.Indices.push_back(1);

		std::vector<Assets::Mesh> customMesh = { mesh };

		return customMesh;
	}

	std::vector<Mesh> MeshGenerator::GenerateGridPlaneMesh(glm::vec3 pos, float cellSize, int width, int height) {
		Assets::Mesh mesh = {};

		float offset_width = 0.0f;
		float offset_height = 0.0f;

		for (size_t x = 0; x < width; x++) {
			for (size_t y = 0; y < height; y++) {

				Assets::Vertex v[4];
				uint32_t i[6];

				v[0].pos = glm::vec3(pos.x + offset_width, pos.y, pos.z + offset_height + cellSize);
				v[1].pos = glm::vec3(pos.x + offset_width + cellSize, pos.y, pos.z + offset_height + cellSize);
				v[2].pos = glm::vec3(pos.x + offset_width, pos.y, pos.z + offset_height);
				v[3].pos = glm::vec3(pos.x + offset_width + cellSize, pos.y, pos.z + offset_height);

				mesh.Vertices.push_back(v[0]);
				mesh.Vertices.push_back(v[1]);
				mesh.Vertices.push_back(v[2]);
				mesh.Vertices.push_back(v[3]);

				i[0] = static_cast<uint32_t>(mesh.Vertices.size() - 2);
				i[1] = static_cast<uint32_t>(mesh.Vertices.size() - 4);
				i[2] = static_cast<uint32_t>(mesh.Vertices.size() - 3);
				i[3] = static_cast<uint32_t>(mesh.Vertices.size() - 1);
				i[4] = static_cast<uint32_t>(mesh.Vertices.size() - 2);
				i[5] = static_cast<uint32_t>(mesh.Vertices.size() - 3);

				mesh.Indices.push_back(i[0]);
				mesh.Indices.push_back(i[1]);
				mesh.Indices.push_back(i[2]);
				mesh.Indices.push_back(i[3]);
				mesh.Indices.push_back(i[4]);
				mesh.Indices.push_back(i[5]);

				offset_height += cellSize;
			}
			
			offset_width += cellSize;
			offset_height = 0.0f;
		}

		std::vector<Assets::Mesh> customMesh = { mesh };

		return customMesh;
	}
}