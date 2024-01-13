#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <map>
#include <stdexcept>

#include "../Material.h"
#include "../LogicalDevice.h"
#include "../PhysicalDevice.h"
#include "../CommandPool.h"
#include "../BufferHelper.h"

#include "../../Assets/Object.h"
#include "../../Assets/Mesh.h"
#include "../../Assets/Texture.h"

#include "./TextureLoader.h"

namespace Engine {
	namespace Utils {
		ModelLoader::ModelLoader() {

		}

		ModelLoader::~ModelLoader() {

		}

		void ModelLoader::LoadModelAndMaterials(
			Assets::Object& object, 
			std::map<std::string, std::unique_ptr<Engine::Material>>& sceneMaterials,
			Engine::LogicalDevice& logicalDevice,
			Engine::PhysicalDevice& physicalDevice,
			Engine::CommandPool& commandPool) {

			tinyobj::attrib_t attributes;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;

			std::string warn;
			std::string error;

			if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &error, object.ModelPath, object.MaterialPath)) {
				throw std::runtime_error(warn + error);
			}

			//if (materials.size() == 0) return;

			std::cout << "Loading model materials..." << '\n';

			std::string modelBasePath = materials.size() > 0 ? object.MaterialPath : "";
			modelBasePath.append("/");

			//for (const auto& material : materials) {
			for (size_t i = 0; i < materials.size(); i++) {
				const auto& material = materials[i];

				if (sceneMaterials.find(material.name) != sceneMaterials.end())
					continue;

				sceneMaterials[material.name].reset(new class Material());

				sceneMaterials[material.name]->Name = material.name;
				sceneMaterials[material.name]->Diffuse = { material.diffuse[0], material.diffuse[1], material.diffuse[2] };
				sceneMaterials[material.name]->Specular = { material.specular[0], material.specular[1], material.specular[2] };
				sceneMaterials[material.name]->Transmittance = { material.transmittance[0], material.transmittance[1], material.transmittance[2] };
				sceneMaterials[material.name]->Emission = { material.emission[0], material.emission[1], material.emission[2] };
				sceneMaterials[material.name]->Shininess = material.shininess;
				sceneMaterials[material.name]->Ior = material.ior;
				sceneMaterials[material.name]->Dissolve = material.dissolve;
				sceneMaterials[material.name]->Roughness = material.roughness;
				sceneMaterials[material.name]->Metallic = material.metallic;
				sceneMaterials[material.name]->Sheen = material.sheen;
				sceneMaterials[material.name]->ClearcoatThickness = material.clearcoat_thickness;
				sceneMaterials[material.name]->ClearcoatRoughness = material.clearcoat_roughness;
				sceneMaterials[material.name]->Anisotropy = material.anisotropy;
				sceneMaterials[material.name]->AnisotropyRotation = material.anisotropy_rotation;
				//sceneMaterials[material.name]->Pad0 = material.pad0;
				sceneMaterials[material.name]->Pad2 = material.pad2;
				sceneMaterials[material.name]->Illum = material.illum;

				if (material.ambient_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::AMBIENT,
							(modelBasePath + material.ambient_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}

				if (material.diffuse_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::DIFFUSE,
							(modelBasePath + material.diffuse_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.specular_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::SPECULAR,
							(modelBasePath + material.specular_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.specular_highlight_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::SPECULAR_HIGHTLIGHT,
							(modelBasePath + material.specular_highlight_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.bump_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::BUMP,
							(modelBasePath + material.bump_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.displacement_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::DISPLACEMENT,
							(modelBasePath + material.displacement_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.alpha_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::ALPHA,
							(modelBasePath + material.alpha_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.reflection_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::REFLECTION,
							(modelBasePath + material.reflection_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.roughness_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::ROUGHNESS,
							(modelBasePath + material.roughness_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.metallic_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::METALLIC,
							(modelBasePath + material.metallic_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.sheen_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::SHEEN,
							(modelBasePath + material.sheen_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.emissive_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::EMISSIVE,
							(modelBasePath + material.emissive_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
				
				if (material.normal_texname != "") {
					sceneMaterials[material.name]->Textures.push_back(
						TextureLoader::CreateTexture(
							Assets::Texture::TextureType::NORMAL,
							(modelBasePath + material.normal_texname).c_str(),
							logicalDevice,
							physicalDevice,
							commandPool
						)
					);
				}
			}

			std::unordered_map<Assets::Vertex, uint32_t> uniqueVertices{};

			std::cout << "Loading model meshes..." << '\n';

			for (const auto& shape : shapes) {
				Assets::Mesh* newMesh = new Assets::Mesh();

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

					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<uint32_t>(newMesh->Vertices.size());

						newMesh->Vertices.push_back(vertex);
					}

					newMesh->Indices.push_back(uniqueVertices[vertex]);
				}

				newMesh->MaterialName = materials.size() == 0 ? "DefaultMaterial" : materials[shape.mesh.material_ids[0]].name;
				newMesh->Material.reset(sceneMaterials.find(newMesh->MaterialName) == sceneMaterials.end() ? nullptr : sceneMaterials.find(newMesh->MaterialName)->second.get());

				BufferHelper::CreateBuffer(MAX_FRAMES_IN_FLIGHT, logicalDevice, physicalDevice, commandPool, 
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newMesh->Vertices, newMesh->VertexBuffer);

				BufferHelper::CreateBuffer(1, logicalDevice, physicalDevice, commandPool, 
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newMesh->Indices, newMesh->IndexBuffer);

				object.Meshes.push_back(newMesh);
			}
		}
	}
}