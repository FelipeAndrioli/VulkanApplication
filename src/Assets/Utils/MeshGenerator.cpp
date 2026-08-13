#include "./MeshGenerator.h"
#include "../Mesh.h"

#include "../../Core/Timestep.h"

#include <iostream>

#define INVALID_TRIANGLE_INDEX -1

//#define QUAD_BOTTOM_LEFT_INDEX 0
//#define QUAD_TOP_LEFT_INDEX 1
//#define QUAD_TOP_RIGHT_INDEX 2
//#define QUAD_BOTTOM_RIGHT_INDEX 3

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
		mesh.Indices.push_back(21);
		mesh.Indices.push_back(18);
		mesh.Indices.push_back(15);
		mesh.Indices.push_back(21);
		mesh.Indices.push_back(15);
		mesh.Indices.push_back(12);

		std::vector<Assets::Mesh> customMesh = { mesh };

		return customMesh;
	}

	std::vector<Mesh> MeshGenerator::GenerateQuadMesh(const glm::vec3 pos, const float size) {
		Assets::Mesh mesh = {};

		mesh.Vertices.resize(4);

		mesh.Vertices[0].pos = glm::vec3(pos.x, pos.y, pos.z);
		mesh.Vertices[1].pos = glm::vec3(pos.x, pos.y, pos.z + size);
		mesh.Vertices[2].pos = glm::vec3(pos.x + size, pos.y, pos.z + size);
		mesh.Vertices[3].pos = glm::vec3(pos.x + size, pos.y, pos.z);
/*
        mesh.Vertices[0].pos = glm::vec3(pos.x, pos.y, pos.z);
		mesh.Vertices[1].pos = glm::vec3(pos.x, pos.y + size, pos.z);
		mesh.Vertices[2].pos = glm::vec3(pos.x + size, pos.y + size, pos.z);
		mesh.Vertices[3].pos = glm::vec3(pos.x + size, pos.y, pos.z);
*/
		mesh.Vertices[0].texCoord = glm::vec2(0.0f, 0.0f);
		mesh.Vertices[1].texCoord = glm::vec2(0.0f, 1.0f);
		mesh.Vertices[2].texCoord = glm::vec2(1.0f, 1.0f);
		mesh.Vertices[3].texCoord = glm::vec2(1.0f, 0.0f);

//		glm::vec3 normal = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

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

	bool MeshGenerator::IsTriangleClockWise(const glm::vec3 A, const glm::vec3 B, const glm::vec3 C) {
		glm::vec2 E1 = glm::vec2(B.x - A.x, B.y - A.y);
		glm::vec2 E2 = glm::vec2(C.x - B.x, C.y - B.y);

		float R = (E1.x * E2.y) - (E1.y * E2.y);

		return R < 0.0f;
	}

	std::vector<Mesh> MeshGenerator::GenerateIcosphereMesh(const size_t Subdivisions) {

		float goldenRatio = (1.0f + glm::sqrt(5.0f)) * 0.5f;

		Assets::Mesh mesh = {};
		mesh.Vertices.resize(12);

		mesh.Vertices[0].pos = glm::normalize(glm::vec3(-1, goldenRatio, 0));
		mesh.Vertices[1].pos = glm::normalize(glm::vec3(1, goldenRatio, 0));
		mesh.Vertices[2].pos = glm::normalize(glm::vec3(-1, -goldenRatio, 0));
		mesh.Vertices[3].pos = glm::normalize(glm::vec3(1, -goldenRatio, 0));
		mesh.Vertices[4].pos = glm::normalize(glm::vec3(0, -1, goldenRatio));
		mesh.Vertices[5].pos = glm::normalize(glm::vec3(0, 1, goldenRatio));
		mesh.Vertices[6].pos = glm::normalize(glm::vec3(0, -1, -goldenRatio));
		mesh.Vertices[7].pos = glm::normalize(glm::vec3(0, 1, -goldenRatio));
		mesh.Vertices[8].pos = glm::normalize(glm::vec3(goldenRatio, 0, -1));
		mesh.Vertices[9].pos = glm::normalize(glm::vec3(goldenRatio, 0 , 1));
		mesh.Vertices[10].pos = glm::normalize(glm::vec3(-goldenRatio, 0, -1));
		mesh.Vertices[11].pos = glm::normalize(glm::vec3(-goldenRatio, 0, 1));

		mesh.Indices.resize(60);
		
		// 5 faces around point 0
		mesh.Indices[0] = 0;
		mesh.Indices[1] = 11;
		mesh.Indices[2] = 5;
		mesh.Indices[3] = 0;
		mesh.Indices[4] = 5;
		mesh.Indices[5] = 1;
		mesh.Indices[6] = 0;
		mesh.Indices[7] = 1;
		mesh.Indices[8] = 7;
		mesh.Indices[9] = 0;
		mesh.Indices[10] = 7;
		mesh.Indices[11] = 10;
		mesh.Indices[12] = 0;
		mesh.Indices[13] = 10;
		mesh.Indices[14] = 11;
	
		// 5 adjacent faces
		mesh.Indices[15] = 1;
		mesh.Indices[16] = 5;
		mesh.Indices[17] = 9;
		mesh.Indices[18] = 5;
		mesh.Indices[19] = 11;
		mesh.Indices[20] = 4;
		mesh.Indices[21] = 11;
		mesh.Indices[22] = 10;
		mesh.Indices[23] = 2;
		mesh.Indices[24] = 10;
		mesh.Indices[25] = 7;
		mesh.Indices[26] = 6;
		mesh.Indices[27] = 7;
		mesh.Indices[28] = 1;
		mesh.Indices[29] = 8;
		
		// 5 faces around point 3
		mesh.Indices[30] = 3;
		mesh.Indices[31] = 9;
		mesh.Indices[32] = 4;
		mesh.Indices[33] = 3;
		mesh.Indices[34] = 4;
		mesh.Indices[35] = 2;
		mesh.Indices[36] = 3;
		mesh.Indices[37] = 2;
		mesh.Indices[38] = 6;
		mesh.Indices[39] = 3;
		mesh.Indices[40] = 6;
		mesh.Indices[41] = 8;
		mesh.Indices[42] = 3;
		mesh.Indices[43] = 8;
		mesh.Indices[44] = 9;

		// 5 adjacent faces
		mesh.Indices[45] = 4;
		mesh.Indices[46] = 9;
		mesh.Indices[47] = 5;
		mesh.Indices[48] = 2;
		mesh.Indices[49] = 4;
		mesh.Indices[50] = 11;
		mesh.Indices[51] = 6;
		mesh.Indices[52] = 2;
		mesh.Indices[53] = 10;
		mesh.Indices[54] = 8;
		mesh.Indices[55] = 6;
		mesh.Indices[56] = 7;
		mesh.Indices[57] = 9;
		mesh.Indices[58] = 8;
		mesh.Indices[59] = 1;

		std::unordered_map<uint32_t, uint32_t> MiddlePointIndexCache;

		for (size_t SubdivisionIndex = 0; SubdivisionIndex < Subdivisions; ++SubdivisionIndex) {
	
			std::vector<uint32_t> SubdividedIndices;

			for (uint32_t TriangleIndex = 0; TriangleIndex < mesh.Indices.size(); TriangleIndex += 3) {

				uint32_t Triangle[3] = { mesh.Indices[TriangleIndex + 0], mesh.Indices[TriangleIndex + 1], mesh.Indices[TriangleIndex + 2] };
				uint32_t NewTriangle[3] = {};

				for (uint32_t InnerTriangleIndex = 0; InnerTriangleIndex < 3; ++InnerTriangleIndex) {
					uint32_t MiddlePointIndex = 0;

					uint32_t IndexA = Triangle[InnerTriangleIndex];
					uint32_t IndexB = Triangle[(InnerTriangleIndex + 1) % 3];

					bool IsFirstSmaller = IndexA < IndexB;

					uint32_t SmallerIndex = IsFirstSmaller ? IndexA : IndexB;
					uint32_t GreaterIndex = IsFirstSmaller ? IndexB : IndexA;

					uint32_t Key = (SmallerIndex << 16) + GreaterIndex;

					if (MiddlePointIndexCache.find(Key) != MiddlePointIndexCache.end()) {
						MiddlePointIndex = MiddlePointIndexCache.find(Key)->second;
					} else {
						glm::vec3 PointA = mesh.Vertices[IndexA].pos;
						glm::vec3 PointB = mesh.Vertices[IndexB].pos;
						glm::vec3 Middle = glm::vec3((PointA.x + PointB.x) / 2.0f, (PointA.y + PointB.y) / 2.0f, (PointA.z + PointB.z) / 2.0f);

						mesh.Vertices.push_back({});
						MiddlePointIndex = mesh.Vertices.size() - 1;
						mesh.Vertices[MiddlePointIndex].pos = glm::normalize(Middle);

						MiddlePointIndexCache[Key] = MiddlePointIndex;
					}

					NewTriangle[InnerTriangleIndex] = MiddlePointIndex;
				}

				for (uint32_t InnerTriangleIndex = 0; InnerTriangleIndex < 3; ++InnerTriangleIndex) {
					SubdividedIndices.push_back(Triangle[InnerTriangleIndex]);
					SubdividedIndices.push_back(NewTriangle[InnerTriangleIndex]);
					SubdividedIndices.push_back(NewTriangle[(InnerTriangleIndex + 2) % 3]);
				}

				SubdividedIndices.push_back(NewTriangle[0]);
				SubdividedIndices.push_back(NewTriangle[1]);
				SubdividedIndices.push_back(NewTriangle[2]);
			}

			mesh.Indices = SubdividedIndices;
		}


		for (size_t VertexIndex = 0; VertexIndex < mesh.Vertices.size(); ++VertexIndex) {
			// Note: All vertices are already normalized.

			// Note: If the sphere center is (0, 0, 0), the surface normal at a vertex
			// is simply its position vector normalized.
		
			mesh.Vertices[VertexIndex].normal = mesh.Vertices[VertexIndex].pos;
		}

		std::vector<Assets::Mesh> icosphereMesh = { mesh };

		return icosphereMesh;
	}

    size_t BuildVertexKey(const glm::vec3 position, const glm::vec3 normal) { 
        std::hash<glm::vec3> hasher;

        return hasher(position) ^ hasher(normal);
    }

    std::vector<Mesh> MeshGenerator::GenerateTriangleIndexedMultiQuadMesh(int verticalVerticesCount, int horizontalVerticesCount, glm::vec3 position, float size) {
        std::cout << "Generating triangle indexed multi quad mesh started!\n";

        const uint32_t QUAD_BOTTOM_LEFT_INDEX = 0;
        const uint32_t QUAD_TOP_LEFT_INDEX = 1;
        const uint32_t QUAD_TOP_RIGHT_INDEX = 2;
        const uint32_t QUAD_BOTTOM_RIGHT_INDEX = 3;

        Timestep startTime = glfwGetTime();

        Assets::Mesh mesh = {};

        // TODO: generate tex coords.
        int verticalVerticesToGenerate = verticalVerticesCount < 2 ? 2 : verticalVerticesCount;
        int horizontalVerticesToGenerate = horizontalVerticesCount < 2 ? 2 : horizontalVerticesCount;

        int verticalQuadsToGenerate = verticalVerticesToGenerate - 1;
        int horizontalQuadsToGenerate = horizontalVerticesToGenerate - 1;

        glm::vec3 quadPosition = position;

        std::unordered_map<size_t, size_t> existingVertices = {};

        for (size_t verticalQuadIndex = 0; verticalQuadIndex < verticalQuadsToGenerate; ++verticalQuadIndex) {
            for (size_t horizontalQuadIndex = 0; horizontalQuadIndex < horizontalQuadsToGenerate; ++horizontalQuadIndex) {

                Assets::Vertex quadVertices[4] = {};

                quadVertices[QUAD_BOTTOM_LEFT_INDEX].pos = glm::vec3(quadPosition.x, quadPosition.y, quadPosition.z);
                quadVertices[QUAD_BOTTOM_LEFT_INDEX].normal = glm::vec3(0.0f, 1.0f, 0.0f);

                quadVertices[QUAD_TOP_LEFT_INDEX].pos = glm::vec3(quadPosition.x, quadPosition.y, quadPosition.z + size);
                quadVertices[QUAD_TOP_LEFT_INDEX].normal = glm::vec3(0.0f, 1.0f, 0.0f);

                quadVertices[QUAD_TOP_RIGHT_INDEX].pos = glm::vec3(quadPosition.x + size, quadPosition.y, quadPosition.z + size);
                quadVertices[QUAD_TOP_RIGHT_INDEX].normal = glm::vec3(0.0f, 1.0f, 0.0f);

                quadVertices[QUAD_BOTTOM_RIGHT_INDEX].pos = glm::vec3(quadPosition.x + size, quadPosition.y, quadPosition.z);
                quadVertices[QUAD_BOTTOM_RIGHT_INDEX].normal = glm::vec3(0.0f, 1.0f, 0.0f);

                size_t verticesKey[4] = {};
                verticesKey[QUAD_BOTTOM_LEFT_INDEX] = BuildVertexKey(quadVertices[QUAD_BOTTOM_LEFT_INDEX].pos, quadVertices[QUAD_BOTTOM_LEFT_INDEX].normal);
                verticesKey[QUAD_TOP_LEFT_INDEX] = BuildVertexKey(quadVertices[QUAD_TOP_LEFT_INDEX].pos, quadVertices[QUAD_TOP_LEFT_INDEX].normal);
                verticesKey[QUAD_TOP_RIGHT_INDEX] = BuildVertexKey(quadVertices[QUAD_TOP_RIGHT_INDEX].pos, quadVertices[QUAD_TOP_RIGHT_INDEX].normal);
                verticesKey[QUAD_BOTTOM_RIGHT_INDEX] = BuildVertexKey(quadVertices[QUAD_BOTTOM_RIGHT_INDEX].pos, quadVertices[QUAD_BOTTOM_RIGHT_INDEX].normal);

                int quadIndices[4] = {};

                quadIndices[QUAD_BOTTOM_LEFT_INDEX] = existingVertices.find(verticesKey[QUAD_BOTTOM_LEFT_INDEX]) == existingVertices.end() ? INVALID_TRIANGLE_INDEX : existingVertices[verticesKey[QUAD_BOTTOM_LEFT_INDEX]];
                quadIndices[QUAD_BOTTOM_RIGHT_INDEX] = existingVertices.find(verticesKey[QUAD_BOTTOM_RIGHT_INDEX]) == existingVertices.end() ? INVALID_TRIANGLE_INDEX : existingVertices[verticesKey[QUAD_BOTTOM_RIGHT_INDEX]];
                quadIndices[QUAD_TOP_LEFT_INDEX] = existingVertices.find(verticesKey[QUAD_TOP_LEFT_INDEX]) == existingVertices.end() ? INVALID_TRIANGLE_INDEX : existingVertices[verticesKey[QUAD_TOP_LEFT_INDEX]];
                quadIndices[QUAD_TOP_RIGHT_INDEX] = existingVertices.find(verticesKey[QUAD_TOP_RIGHT_INDEX]) == existingVertices.end() ? INVALID_TRIANGLE_INDEX : existingVertices[verticesKey[QUAD_TOP_RIGHT_INDEX]];

                if (quadIndices[QUAD_BOTTOM_LEFT_INDEX] == INVALID_TRIANGLE_INDEX) {
                    quadIndices[QUAD_BOTTOM_LEFT_INDEX] = mesh.Vertices.size();
                    existingVertices[verticesKey[QUAD_BOTTOM_LEFT_INDEX]] = quadIndices[QUAD_BOTTOM_LEFT_INDEX];
                    mesh.Vertices.push_back(quadVertices[QUAD_BOTTOM_LEFT_INDEX]);
                }

                if (quadIndices[QUAD_TOP_LEFT_INDEX] == INVALID_TRIANGLE_INDEX) {
                    quadIndices[QUAD_TOP_LEFT_INDEX] = mesh.Vertices.size();
                    existingVertices[verticesKey[QUAD_TOP_LEFT_INDEX]] = quadIndices[QUAD_TOP_LEFT_INDEX];
                    mesh.Vertices.push_back(quadVertices[QUAD_TOP_LEFT_INDEX]);
                }

                if (quadIndices[QUAD_TOP_RIGHT_INDEX] == INVALID_TRIANGLE_INDEX) {
                    quadIndices[QUAD_TOP_RIGHT_INDEX] = mesh.Vertices.size();
                    existingVertices[verticesKey[QUAD_TOP_RIGHT_INDEX]] = quadIndices[QUAD_TOP_RIGHT_INDEX];
                    mesh.Vertices.push_back(quadVertices[QUAD_TOP_RIGHT_INDEX]);
                }

                if (quadIndices[QUAD_BOTTOM_RIGHT_INDEX] == INVALID_TRIANGLE_INDEX) {
                    quadIndices[QUAD_BOTTOM_RIGHT_INDEX] = mesh.Vertices.size();
                    existingVertices[verticesKey[QUAD_BOTTOM_RIGHT_INDEX]] = quadIndices[QUAD_BOTTOM_RIGHT_INDEX];
                    mesh.Vertices.push_back(quadVertices[QUAD_BOTTOM_RIGHT_INDEX]);
                }

                mesh.Indices.push_back(quadIndices[QUAD_BOTTOM_LEFT_INDEX]);
                mesh.Indices.push_back(quadIndices[QUAD_TOP_LEFT_INDEX]);
                mesh.Indices.push_back(quadIndices[QUAD_TOP_RIGHT_INDEX]);
                // Note: When generating indexed quads one of the indices must be duplicated.
//                mesh.Indices.push_back(quadIndices[QUAD_TOP_RIGHT_INDEX]);

                mesh.Indices.push_back(quadIndices[QUAD_TOP_RIGHT_INDEX]);
                mesh.Indices.push_back(quadIndices[QUAD_BOTTOM_RIGHT_INDEX]);
                mesh.Indices.push_back(quadIndices[QUAD_BOTTOM_LEFT_INDEX]);
//                mesh.Indices.push_back(quadIndices[QUAD_BOTTOM_LEFT_INDEX]);

                quadPosition.x += size;
            }

            quadPosition.x = position.x;
            quadPosition.z += size;
        }

        std::vector<Assets::Mesh> result = { mesh };
        Timestep finishTime = glfwGetTime();

        std::cout << "Generating triangle indexed multi quad mesh finished in " << finishTime.GetSeconds() - startTime.GetSeconds() << " seconds\n";

        return result;
    }
}
