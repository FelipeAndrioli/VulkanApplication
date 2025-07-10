#include "./MeshGenerator.h"
#include "../Mesh.h"

namespace Assets {
	glm::vec3 MeshGenerator::GenerateTangentVector(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3) {
		
		glm::vec3 edge1 = p2 - p1;
		glm::vec3 edge2 = p3 - p1;

		glm::vec2 deltaUv1 = uv2 - uv1;
		glm::vec2 deltaUv2 = uv3 - uv1;

		float f = 1 / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x);

		return (f * (deltaUv2.y * edge1 - deltaUv1.y * edge2));
	}

	glm::vec3 MeshGenerator::GenerateBiTangentVector(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3) {
		glm::vec3 edge1 = p2 - p1;
		glm::vec3 edge2 = p3 - p1;

		glm::vec2 deltaUv1 = uv2 - uv1;
		glm::vec2 deltaUv2 = uv3 - uv1;

		float f = 1 / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x);

		return (f * (deltaUv1.x * edge2 - deltaUv2.x * edge1));
	}

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
		mesh.Vertices.resize(24);
	
		const glm::vec3 NormalX = glm::vec3(1.0f, 0.0f, 0.0f);
		const glm::vec3 NormalY = glm::vec3(0.0f, 1.0f, 0.0);
		const glm::vec3 NormalZ = glm::vec3(0.0f, 0.0f, 1.0f);

		mesh.Vertices[0].pos = glm::vec3(pos.x, pos.y, pos.z);
		mesh.Vertices[0].normal = -NormalZ;
		mesh.Vertices[1].pos = glm::vec3(pos.x, pos.y, pos.z);							
		mesh.Vertices[1].normal = -NormalY;
		mesh.Vertices[2].pos = glm::vec3(pos.x, pos.y, pos.z);							
		mesh.Vertices[2].normal = -NormalX;

		mesh.Vertices[3].pos = glm::vec3(pos.x, pos.y + size, pos.z);
		mesh.Vertices[3].normal = -NormalZ;
		mesh.Vertices[4].pos = glm::vec3(pos.x, pos.y + size, pos.z);
		mesh.Vertices[4].normal = NormalY;
		mesh.Vertices[5].pos = glm::vec3(pos.x, pos.y + size, pos.z);
		mesh.Vertices[5].normal = -NormalX;

		mesh.Vertices[6].pos = glm::vec3(pos.x + size, pos.y + size, pos.z);
		mesh.Vertices[6].normal = -NormalZ;
		mesh.Vertices[7].pos = glm::vec3(pos.x + size, pos.y + size, pos.z);
		mesh.Vertices[7].normal = NormalY;
		mesh.Vertices[8].pos = glm::vec3(pos.x + size, pos.y + size, pos.z);
		mesh.Vertices[8].normal = NormalX;

		mesh.Vertices[9].pos = glm::vec3(pos.x + size, pos.y, pos.z);
		mesh.Vertices[9].normal = -NormalZ;
		mesh.Vertices[10].pos = glm::vec3(pos.x + size, pos.y, pos.z);
		mesh.Vertices[10].normal = -NormalY;
		mesh.Vertices[11].pos = glm::vec3(pos.x + size, pos.y, pos.z);
		mesh.Vertices[11].normal = NormalX;

		mesh.Vertices[12].pos = glm::vec3(pos.x, pos.y, pos.z + size);
		mesh.Vertices[12].normal = NormalZ;
		mesh.Vertices[13].pos = glm::vec3(pos.x, pos.y, pos.z + size);
		mesh.Vertices[13].normal = -NormalY;
		mesh.Vertices[14].pos = glm::vec3(pos.x, pos.y, pos.z + size);
		mesh.Vertices[14].normal = -NormalX;

		mesh.Vertices[15].pos = glm::vec3(pos.x, pos.y + size, pos.z + size);
		mesh.Vertices[15].normal = NormalZ;
		mesh.Vertices[16].pos = glm::vec3(pos.x, pos.y + size, pos.z + size);
		mesh.Vertices[16].normal = NormalY;
		mesh.Vertices[17].pos = glm::vec3(pos.x, pos.y + size, pos.z + size);
		mesh.Vertices[17].normal = -NormalX;

		mesh.Vertices[18].pos = glm::vec3(pos.x + size, pos.y + size, pos.z + size);
		mesh.Vertices[18].normal = NormalZ;
		mesh.Vertices[19].pos = glm::vec3(pos.x + size, pos.y + size, pos.z + size);
		mesh.Vertices[19].normal = NormalY;
		mesh.Vertices[20].pos = glm::vec3(pos.x + size, pos.y + size, pos.z + size);
		mesh.Vertices[20].normal = NormalX;

		mesh.Vertices[21].pos = glm::vec3(pos.x + size, pos.y, pos.z + size);			
		mesh.Vertices[21].normal = NormalZ;
		mesh.Vertices[22].pos = glm::vec3(pos.x + size, pos.y, pos.z + size);			
		mesh.Vertices[22].normal = -NormalY;
		mesh.Vertices[23].pos = glm::vec3(pos.x + size, pos.y, pos.z + size);			
		mesh.Vertices[23].normal = NormalX;

		// Front Face
		mesh.Indices.push_back(0);
		mesh.Indices.push_back(3);
		mesh.Indices.push_back(6);
		mesh.Indices.push_back(0);
		mesh.Indices.push_back(6);
		mesh.Indices.push_back(9);

		// Right Face
		mesh.Indices.push_back(11);
		mesh.Indices.push_back(8);
		mesh.Indices.push_back(20);
		mesh.Indices.push_back(11);
		mesh.Indices.push_back(20);
		mesh.Indices.push_back(23);

		// Left Face
		mesh.Indices.push_back(14);
		mesh.Indices.push_back(17);
		mesh.Indices.push_back(5);
		mesh.Indices.push_back(14);
		mesh.Indices.push_back(5);
		mesh.Indices.push_back(2);
		
		// Top Face
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(16);
		mesh.Indices.push_back(19);
		mesh.Indices.push_back(4);
		mesh.Indices.push_back(19);
		mesh.Indices.push_back(7);

		// Bottom Face
		mesh.Indices.push_back(13);
		mesh.Indices.push_back(1);
		mesh.Indices.push_back(10);
		mesh.Indices.push_back(13);
		mesh.Indices.push_back(10);
		mesh.Indices.push_back(22);

		// Back Face
		mesh.Indices.push_back(12);
		mesh.Indices.push_back(15);
		mesh.Indices.push_back(18);
		mesh.Indices.push_back(12);
		mesh.Indices.push_back(18);
		mesh.Indices.push_back(21);

		std::vector<Assets::Mesh> customMesh = { mesh };

		return customMesh;
	}

	std::vector<Mesh> MeshGenerator::GenerateQuadMesh(const glm::vec3 pos, const float size) {
		Assets::Mesh mesh = {};

		mesh.Vertices.resize(4);

		mesh.Vertices[0].pos = glm::vec3(pos.x, pos.y, pos.z);
		mesh.Vertices[1].pos = glm::vec3(pos.x, pos.y + size, pos.z);
		mesh.Vertices[2].pos = glm::vec3(pos.x + size, pos.y + size, pos.z);
		mesh.Vertices[3].pos = glm::vec3(pos.x + size, pos.y, pos.z);

		mesh.Vertices[0].texCoord = glm::vec2(0.0f, 0.0f);
		mesh.Vertices[1].texCoord = glm::vec2(0.0f, 1.0f);
		mesh.Vertices[2].texCoord = glm::vec2(1.0f, 1.0f);
		mesh.Vertices[3].texCoord = glm::vec2(1.0f, 0.0f);

		glm::vec3 normal = glm::vec3(0.0f, 0.0f, -1.0f);

		mesh.Vertices[0].normal = normal;
		mesh.Vertices[1].normal = normal;
		mesh.Vertices[2].normal = normal;
		mesh.Vertices[3].normal = normal;

		mesh.Indices.resize(6);

		mesh.Indices[0] = 0;
		mesh.Indices[1] = 1;
		mesh.Indices[2] = 2;
		mesh.Indices[3] = 0;
		mesh.Indices[4] = 2;
		mesh.Indices[5] = 3;

		glm::vec3 tangent = GenerateTangentVector(
			mesh.Vertices[0].pos, 
			mesh.Vertices[1].pos, 
			mesh.Vertices[2].pos, 
			mesh.Vertices[0].texCoord, 
			mesh.Vertices[1].texCoord, 
			mesh.Vertices[2].texCoord);

		mesh.Vertices[0].tangent = tangent;
		mesh.Vertices[1].tangent = tangent;
		mesh.Vertices[2].tangent = tangent;

		tangent = GenerateTangentVector(
			mesh.Vertices[0].pos, 
			mesh.Vertices[2].pos, 
			mesh.Vertices[3].pos, 
			mesh.Vertices[0].texCoord, 
			mesh.Vertices[2].texCoord, 
			mesh.Vertices[3].texCoord);

		mesh.Vertices[0].tangent = tangent;
		mesh.Vertices[2].tangent = tangent;
		mesh.Vertices[3].tangent = tangent;

		std::vector<Assets::Mesh> quadMesh = { mesh };

		return quadMesh;
	}
}
