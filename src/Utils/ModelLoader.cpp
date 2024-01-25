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
			std::map<std::string, std::unique_ptr<Assets::Texture>>& loadedTextures,
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

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::AMBIENT, 
					material.ambient_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::AMBIENT,
					material.ambient_texname,
					material.name
				);

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::DIFFUSE, 
					material.diffuse_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::DIFFUSE,
					material.diffuse_texname,
					material.name
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::DIFFUSE,
					"error_texture.jpg",
					material.name
				);

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::DIFFUSE, 
					"error_texture.jpg",
					"./Assets/Textures/",
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					false
				);

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::SPECULAR, 
					material.specular_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::SPECULAR,
					material.specular_texname,
					material.name
				);

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::SPECULAR_HIGHTLIGHT, 
					material.specular_highlight_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::SPECULAR_HIGHTLIGHT,
					material.specular_highlight_texname,
					material.name
				);

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::SPECULAR_HIGHTLIGHT, 
					material.specular_highlight_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::SPECULAR_HIGHTLIGHT,
					material.specular_highlight_texname,
					material.name
				);
	
				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::BUMP, 
					material.bump_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::BUMP,
					material.bump_texname,
					material.name
				);
					
				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::DISPLACEMENT, 
					material.displacement_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::DISPLACEMENT,
					material.displacement_texname,
					material.name
				);			

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::ALPHA, 
					material.alpha_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::ALPHA,
					material.alpha_texname,
					material.name
				);
	
				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::REFLECTION, 
					material.reflection_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::REFLECTION,
					material.reflection_texname,
					material.name
				);			

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::ROUGHNESS, 
					material.roughness_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::ROUGHNESS,
					material.roughness_texname,
					material.name
				);			

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::METALLIC, 
					material.metallic_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::METALLIC,
					material.metallic_texname,
					material.name
				);
	
				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::SHEEN, 
					material.sheen_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::SHEEN,
					material.sheen_texname,
					material.name
				);
	
				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::EMISSIVE, 
					material.emissive_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::EMISSIVE,
					material.emissive_texname,
					material.name
				);			

				ModelLoader::ValidateAndInsertTexture(
					loadedTextures, 
					Assets::TextureType::NORMAL, 
					material.normal_texname,
					modelBasePath, 
					logicalDevice, 
					physicalDevice, 
					commandPool, 
					object.FlipTexturesVertically
				);

				ModelLoader::LoadTextureToMaterial(
					sceneMaterials,
					loadedTextures,
					Assets::TextureType::NORMAL,
					material.normal_texname,
					material.name
				);
			}

			std::cout << "Loading model meshes..." << '\n';

			for (const auto& shape : shapes) {
				std::unordered_map<Assets::Vertex, uint32_t> uniqueVertices{};

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

		void ModelLoader::ValidateAndInsertTexture(
				std::map<std::string, std::unique_ptr<Assets::Texture>>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool,
				bool flipTexturesVertically
			) {

			if (loadedTextures.find(textureName) != loadedTextures.end())
				return;

			if (textureName != "" && fileExists(basePath + textureName)) {
				loadedTextures[textureName].reset(new struct Assets::Texture(
					TextureLoader::CreateTexture(
						textureType,
						(basePath + textureName).c_str(),
						logicalDevice,
						physicalDevice,
						commandPool,
						flipTexturesVertically
					)
				));
			}
		}

		void ModelLoader::LoadTextureToMaterial(
			std::map<std::string, std::unique_ptr<Assets::Material>>& sceneMaterials,
			std::map<std::string, std::unique_ptr<Assets::Texture>>& loadedTextures,
			Assets::TextureType textureType,
			std::string textureName,
			std::string materialName
		) {
			if (loadedTextures.find(textureName) != loadedTextures.end()) {
				sceneMaterials[materialName]->Textures.insert({
						textureType,
						loadedTextures.find(textureName)->second.get()
					}
				);
			}
		}
	}
}