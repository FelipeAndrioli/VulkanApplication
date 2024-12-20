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

	std::vector<Mesh> MeshGenerator::GenerateDisconnectedPlaneMesh(glm::vec3 pos, float cellSize, size_t planeSize) {
		// Same position but not sharing vertices
		// there is a name for that

		Assets::Mesh mesh = {};

		float offset_width = 0.0f;
		float offset_height = 0.0f;

		int v = 0;
		for (size_t x = 0; x < planeSize; x++) {
			for (size_t y = 0; y < planeSize; y++) {

				Assets::Vertex vertex[4];
				uint32_t i[6];

				float pos_y = pos.y;

				vertex[0].pos = glm::vec3(pos.x + offset_width, pos_y, pos.z + offset_height);							// bottom left
				vertex[1].pos = glm::vec3(pos.x + offset_width, pos_y, pos.z + offset_height + cellSize);				// top left
				vertex[2].pos = glm::vec3(pos.x + offset_width + cellSize, pos_y, pos.z + offset_height + cellSize);	// top right 
				vertex[3].pos = glm::vec3(pos.x + offset_width + cellSize, pos_y, pos.z + offset_height);				// bottom right 

				mesh.Vertices.push_back(vertex[0]);
				mesh.Vertices.push_back(vertex[1]);
				mesh.Vertices.push_back(vertex[2]);
				mesh.Vertices.push_back(vertex[3]);

				i[0] = v + 0;
				i[1] = v + 1;
				i[2] = v + 2;
				i[3] = v + 0;
				i[4] = v + 2;
				i[5] = v + 3;

				mesh.Indices.push_back(i[0]);
				mesh.Indices.push_back(i[1]);
				mesh.Indices.push_back(i[2]);
				mesh.Indices.push_back(i[3]);
				mesh.Indices.push_back(i[4]);
				mesh.Indices.push_back(i[5]);

				offset_height += cellSize;
				v += 4;
			}
	
			offset_width += cellSize;
			offset_height = 0.0f;
		}

		std::vector<Assets::Mesh> customMesh = { mesh };

		return customMesh;
	}

	std::vector<Mesh> MeshGenerator::GeneratePlaneMesh(glm::vec3 pos, float cellSize, size_t planeSize) {
		// Sharing vertices

		Assets::Mesh mesh = {};

		float offset_width = cellSize;
		float offset_height = cellSize;

		for (size_t x = 0; x <= planeSize; x++) {
			for (size_t y = 0; y <= planeSize; y++) {
				Assets::Vertex vertex = {};
				vertex.pos = glm::vec3(pos.x + offset_width - cellSize, pos.y, pos.z + offset_height - cellSize);

				mesh.Vertices.push_back(vertex);

				offset_height += cellSize;
			}
			offset_width += cellSize;
			offset_height = cellSize;
		}

		int v = 0;

		for (size_t x = 0; x < planeSize; x++) {
			for (size_t y = 0; y < planeSize; y++) {
				uint32_t i[6];

				// vertices are being ordered from left to right, therefore it's not possible just to add + 1 to achieve 
				// an upper row, we need to add the row size + x (x being the column position in upper row).

				i[0] = static_cast<uint32_t>(v + 0);
				i[1] = static_cast<uint32_t>(v + planeSize + 1);
				i[2] = static_cast<uint32_t>(v + planeSize + 2);
				i[3] = static_cast<uint32_t>(v + 0);
				i[4] = static_cast<uint32_t>(v + planeSize + 2);
				i[5] = static_cast<uint32_t>(v + 1);

				mesh.Indices.push_back(i[0]);
				mesh.Indices.push_back(i[1]);
				mesh.Indices.push_back(i[2]);
				mesh.Indices.push_back(i[3]);
				mesh.Indices.push_back(i[4]);
				mesh.Indices.push_back(i[5]);
				
				v++;
			}
			v++;
		}

		std::vector<Assets::Mesh> customMesh = { mesh };

		return customMesh;
	}

	std::vector<Mesh> MeshGenerator::GenerateCubeMesh(glm::vec3 pos, float size) {
		// TODO: rework when implementing voxels

		Assets::Mesh mesh = {};

		Assets::Vertex vertex[8];

		vertex[0].pos = glm::vec3(pos.x, pos.y, pos.z);
		vertex[1].pos = glm::vec3(pos.x, pos.y + size, pos.z);
		vertex[2].pos = glm::vec3(pos.x + size, pos.y + size, pos.z);
		vertex[3].pos = glm::vec3(pos.x + size, pos.y, pos.z);
		vertex[4].pos = glm::vec3(pos.x, pos.y, pos.z + size);
		vertex[5].pos = glm::vec3(pos.x, pos.y + size, pos.z + size);
		vertex[6].pos = glm::vec3(pos.x + size, pos.y + size, pos.z + size);
		vertex[7].pos = glm::vec3(pos.x + size, pos.y, pos.z + size);

		mesh.Vertices.push_back(vertex[0]);
		mesh.Vertices.push_back(vertex[1]);
		mesh.Vertices.push_back(vertex[2]);
		mesh.Vertices.push_back(vertex[3]);
		mesh.Vertices.push_back(vertex[4]);
		mesh.Vertices.push_back(vertex[5]);
		mesh.Vertices.push_back(vertex[6]);
		mesh.Vertices.push_back(vertex[7]);

		// front face
		mesh.Indices.push_back(0);
		mesh.Indices.push_back(1);
		mesh.Indices.push_back(2);
		mesh.Indices.push_back(0);
		mesh.Indices.push_back(2);
		mesh.Indices.push_back(3);

		// back face
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(6);
		mesh.Indices.push_back(5);
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(7);
		mesh.Indices.push_back(6);

		// left face
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(5);
		mesh.Indices.push_back(1);
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(1);
		mesh.Indices.push_back(0);

		// right face
		mesh.Indices.push_back(3);
		mesh.Indices.push_back(2);
		mesh.Indices.push_back(6);
		mesh.Indices.push_back(3);
		mesh.Indices.push_back(6);
		mesh.Indices.push_back(7);

		// top face
		mesh.Indices.push_back(1);
		mesh.Indices.push_back(5);
		mesh.Indices.push_back(6);
		mesh.Indices.push_back(1);
		mesh.Indices.push_back(6);
		mesh.Indices.push_back(2);

		// bottom face
		mesh.Indices.push_back(3);
		mesh.Indices.push_back(7);
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(3);
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(0);

		std::vector<Assets::Mesh> customMesh = { mesh };

		return customMesh;
	}
}
