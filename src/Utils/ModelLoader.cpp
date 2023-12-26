#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <stdexcept>

#include "../../Assets/Object.h"
#include "../../Assets/Mesh.h"

namespace Engine {
	namespace Utils {
		ModelLoader::ModelLoader() {

		}

		ModelLoader::~ModelLoader() {

		}

		void ModelLoader::LoadModel(Assets::Object& object) {
			tinyobj::attrib_t attributes;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;

			std::string warn;
			std::string error;

			if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &error, object.ModelPath)) {
				throw std::runtime_error(warn + error);
			}

			std::unordered_map<Assets::Vertex, uint32_t> uniqueVertices{};

			for (const auto& shape : shapes) {
				for (const auto& index : shape.mesh.indices) {
					Assets::Vertex vertex{};

					vertex.pos = {
						attributes.vertices[3 * index.vertex_index + 0],
						attributes.vertices[3 * index.vertex_index + 1],
						attributes.vertices[3 * index.vertex_index + 2]
					};

					vertex.texCoord = {
						attributes.texcoords[2 * index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
					};

					vertex.color = { 1.0f, 1.0f, 1.0f };

					object.Meshes->Vertices.push_back(vertex);
					object.Meshes->Indices.push_back(static_cast<uint32_t>(object.Meshes->Indices.size()));

					/*
					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<uint32_t>(object.Meshes->Indices.size());

						object.Meshes->Vertices.push_back(vertex);
					}

					object.Meshes->Indices.push_back(uniqueVertices[vertex]);
					*/
				}
			}
		}
	}
}