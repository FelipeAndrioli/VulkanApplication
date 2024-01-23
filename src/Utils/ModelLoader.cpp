#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <map>
#include <stdexcept>

#include "../LogicalDevice.h"
#include "../PhysicalDevice.h"
#include "../CommandPool.h"
#include "../BufferHelper.h"
#include "../Buffer.h"
#include "../DescriptorSets.h"

#include "../../Assets/Object.h"
#include "../../Assets/Mesh.h"
#include "../../Assets/Texture.h"
#include "../../Assets/Material.h"

#include "./TextureLoader.h"

namespace Engine {
	namespace Utils {
		ModelLoader::ModelLoader() {

		}

		ModelLoader::~ModelLoader() {

		}

		void ModelLoader::LoadModelAndMaterials(
			Assets::Object& object, 
			std::map<std::string, std::unique_ptr<Assets::Material>>& sceneMaterials,
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

			std::cout << "Loading model materials..." << '\n';

			std::string modelBasePath = materials.size() > 0 ? object.MaterialPath : "";
			modelBasePath.append("/");

			for (size_t i = 0; i < materials.size(); i++) {
				const auto& material = materials[i];

				if (sceneMaterials.find(material.name) != sceneMaterials.end())
					continue;

				sceneMaterials[material.name].reset(new class Assets::Material());

				sceneMaterials[material.name]->Properties.Name = material.name;
				sceneMaterials[material.name]->Properties.Diffuse = { material.diffuse[0], material.diffuse[1], material.diffuse[2] };
				sceneMaterials[material.name]->Properties.Specular = { material.specular[0], material.specular[1], material.specular[2] };
				sceneMaterials[material.name]->Properties.Transmittance = { material.transmittance[0], material.transmittance[1], material.transmittance[2] };
				sceneMaterials[material.name]->Properties.Emission = { material.emission[0], material.emission[1], material.emission[2] };
				sceneMaterials[material.name]->Properties.Shininess = material.shininess;
				sceneMaterials[material.name]->Properties.Ior = material.ior;
				sceneMaterials[material.name]->Properties.Dissolve = material.dissolve;
				sceneMaterials[material.name]->Properties.Roughness = material.roughness;
				sceneMaterials[material.name]->Properties.Metallic = material.metallic;
				sceneMaterials[material.name]->Properties.Sheen = material.sheen;
				sceneMaterials[material.name]->Properties.ClearcoatThickness = material.clearcoat_thickness;
				sceneMaterials[material.name]->Properties.ClearcoatRoughness = material.clearcoat_roughness;
				sceneMaterials[material.name]->Properties.Anisotropy = material.anisotropy;
				sceneMaterials[material.name]->Properties.AnisotropyRotation = material.anisotropy_rotation;
				//sceneMaterials[material.name]->Properties.Pad0 = material.pad0;
				sceneMaterials[material.name]->Properties.Pad2 = material.pad2;
				sceneMaterials[material.name]->Properties.Illum = material.illum;

				if (material.ambient_texname != "" && fileExists(modelBasePath + material.ambient_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::AMBIENT,
							TextureLoader::CreateTexture(
								Assets::TextureType::AMBIENT,
								(modelBasePath + material.ambient_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}

				if (material.diffuse_texname != "" && fileExists(modelBasePath + material.diffuse_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::DIFFUSE,
							TextureLoader::CreateTexture(
								Assets::TextureType::DIFFUSE,
								(modelBasePath + material.diffuse_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.specular_texname != "" && fileExists(modelBasePath + material.specular_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::SPECULAR,
							TextureLoader::CreateTexture(
								Assets::TextureType::SPECULAR,
								(modelBasePath + material.specular_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.specular_highlight_texname != "" && fileExists(modelBasePath + material.specular_highlight_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::SPECULAR_HIGHTLIGHT,
							TextureLoader::CreateTexture(
								Assets::TextureType::SPECULAR_HIGHTLIGHT,
								(modelBasePath + material.specular_highlight_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.bump_texname != "" && fileExists(modelBasePath + material.bump_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::BUMP,
							TextureLoader::CreateTexture(
								Assets::TextureType::BUMP,
								(modelBasePath + material.bump_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.displacement_texname != "" && fileExists(modelBasePath + material.displacement_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::DISPLACEMENT,
							TextureLoader::CreateTexture(
								Assets::TextureType::DISPLACEMENT,
								(modelBasePath+ material.displacement_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.alpha_texname != "" && fileExists(modelBasePath + material.alpha_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::ALPHA,
							TextureLoader::CreateTexture(
								Assets::TextureType::ALPHA,
								(modelBasePath + material.alpha_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.reflection_texname != "" && fileExists(modelBasePath + material.reflection_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::REFLECTION,
							TextureLoader::CreateTexture(
								Assets::TextureType::REFLECTION,
								(modelBasePath + material.reflection_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.roughness_texname != "" && fileExists(modelBasePath + material.roughness_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::ROUGHNESS,
							TextureLoader::CreateTexture(
								Assets::TextureType::ROUGHNESS,
								(modelBasePath + material.roughness_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.metallic_texname != "" && fileExists(modelBasePath + material.metallic_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::METALLIC,
							TextureLoader::CreateTexture(
								Assets::TextureType::METALLIC,
								(modelBasePath + material.metallic_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.sheen_texname != "" && fileExists(modelBasePath + material.sheen_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::SHEEN,
							TextureLoader::CreateTexture(
								Assets::TextureType::SHEEN,
								(modelBasePath + material.sheen_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.emissive_texname != "" && fileExists(modelBasePath + material.emissive_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::EMISSIVE,
							TextureLoader::CreateTexture(
								Assets::TextureType::EMISSIVE,
								(modelBasePath + material.emissive_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}
				
				if (material.normal_texname != "" && fileExists(modelBasePath + material.normal_texname)) {
					sceneMaterials[material.name]->Textures.insert(
						std::make_pair<Assets::TextureType, Assets::Texture>(Assets::TextureType::NORMAL,
							TextureLoader::CreateTexture(
								Assets::TextureType::NORMAL,
								(modelBasePath + material.normal_texname).c_str(),
								logicalDevice,
								physicalDevice,
								commandPool,
								object.FlipTexturesVertically
							)
						)
					);
				}

				/*
				VkDeviceSize bufferSize = sizeof(Assets::Material::MaterialProperties);

				sceneMaterials[material.name]->GPUDataBuffer.reset(
					new class Engine::Buffer(
						MAX_FRAMES_IN_FLIGHT,
						logicalDevice,
						physicalDevice,
						bufferSize,
						VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
					)
				);
				sceneMaterials[material.name]->GPUDataBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				sceneMaterials[material.name]->GPUDataBuffer->BufferMemory->MapMemory();

				sceneMaterials[material.name]->DescriptorSets.reset(
					new class Engine::DescriptorSets(
						bufferSize,
						logicalDevice.GetHandle(),
						descriptorPool
					)
				);
				*/
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