#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <unordered_map>
#include <stdexcept>

#include <sys/stat.h>

#include "../Vulkan.h"

#include "../../Assets/Object.h"
#include "../../Assets/Mesh.h"
#include "../../Assets/Texture.h"
#include "../../Assets/Material.h"

#include "./TextureLoader.h"

#include <algorithm>
#include <execution>

constexpr auto UNEXISTENT = -1;

namespace Engine {
	namespace Utils {
		ModelLoader::ModelLoader() {

		}

		ModelLoader::~ModelLoader() {

		}

		void ModelLoader::LoadModelAndMaterials(
			Assets::Object& object, 
			std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures,
			VulkanEngine& vulkanEngine) {

			//TinyLoad(object, sceneMaterials, loadedTextures, vulkanEngine);

			const aiScene* scene = aiImportFile(object.ModelPath, aiProcess_Triangulate);

			if (!scene || !scene->HasMeshes()) {
				throw std::runtime_error("Unable to load file!");
			}

			ProcessNode(object, vulkanEngine, scene->mRootNode, scene, sceneMaterials, loadedTextures);

			for (auto& mesh : object.Meshes) {
				if (GetMaterialIndex(sceneMaterials, mesh.MaterialName) == UNEXISTENT) {
					sceneMaterials.push_back(mesh.CustomMeshMaterial);
				}

				mesh.MaterialIndex = GetMaterialIndex(sceneMaterials, mesh.MaterialName);
			}
		}

		void ModelLoader::ProcessNode(
			Assets::Object& object, 
			VulkanEngine& vulkanEngine,
			const aiNode* node, 
			const aiScene* scene, 
			std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures
		) {
			for (size_t i = 0; i < node->mNumMeshes; i++) {
				const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				object.Meshes.push_back(ProcessMesh(object, vulkanEngine, mesh, scene, sceneMaterials, loadedTextures));
			}

			for (size_t i = 0; i < node->mNumChildren; i++) {
				ProcessNode(object, vulkanEngine, node->mChildren[i], scene, sceneMaterials, loadedTextures);
			}
		}

		Assets::Mesh ModelLoader::ProcessMesh(
			Assets::Object& object,
			VulkanEngine& vulkanEngine,
			const aiMesh* mesh,
			const aiScene* scene,
			std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures
		) {
			Assets::Mesh newMesh = {};
			std::vector<Assets::Vertex> vertices;
			std::vector<uint32_t> indices;

			for (size_t i = 0; i < mesh->mNumVertices; i++) {
				Assets::Vertex vertex = {};
				vertex.pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

				if (mesh->mTextureCoords[0]) {
					vertex.texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
				}
				else {
					vertex.texCoord = { 0.0f, 0.0f };
				}

				vertices.push_back(vertex);
			}

			for (size_t i = 0; i < mesh->mNumFaces; i++) {
				const aiFace face = mesh->mFaces[i];

				for (size_t j = 0; j < face.mNumIndices; j++) {
					indices.push_back(face.mIndices[j]);
				}
			}

			newMesh.Vertices = vertices;
			newMesh.Indices = indices;

			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			aiString materialName;
			aiColor3D diffuseColor;
			aiColor3D specularColor;
			aiColor3D ambientColor;
			aiColor3D emissiveColor;
			aiColor3D transparentColor;
			float opacity;
			float shininess;
			float shininessStrength;

			material->Get(AI_MATKEY_NAME, materialName);
			material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
			material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
			material->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
			material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
			material->Get(AI_MATKEY_COLOR_TRANSPARENT, transparentColor);
			material->Get(AI_MATKEY_OPACITY, opacity);
			material->Get(AI_MATKEY_SHININESS, shininess);
			material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);

			if (GetMaterialIndex(sceneMaterials, materialName.C_Str()) == UNEXISTENT) {
				Assets::Material newMaterial = {};
				newMaterial.Name = materialName.C_Str();
				newMaterial.MaterialData.Ambient = glm::vec4(ambientColor.r, ambientColor.g, ambientColor.b, 1.0f);
				newMaterial.MaterialData.Diffuse = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f);
				newMaterial.MaterialData.Specular = glm::vec4(specularColor.r, specularColor.g, specularColor.b, 1.0f);
				newMaterial.MaterialData.Emission = glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, 1.0f);
				newMaterial.MaterialData.Transparency = glm::vec4(transparentColor.r, transparentColor.g, transparentColor.b, 1.0f);
				newMaterial.MaterialData.Opacity = opacity;
				newMaterial.MaterialData.Shininess = shininess;
				newMaterial.MaterialData.ShininessStrength = shininessStrength;

				sceneMaterials.push_back(newMaterial);
			}

			LoadTextures(object, vulkanEngine, material, aiTextureType_AMBIENT, Assets::TextureType::AMBIENT, sceneMaterials, loadedTextures);
			LoadTextures(object, vulkanEngine, material, aiTextureType_DIFFUSE, Assets::TextureType::DIFFUSE, sceneMaterials, loadedTextures);
			LoadTextures(object, vulkanEngine, material, aiTextureType_SPECULAR, Assets::TextureType::SPECULAR, sceneMaterials, loadedTextures);
			LoadTextures(object, vulkanEngine, material, aiTextureType_NORMALS, Assets::TextureType::NORMAL, sceneMaterials, loadedTextures);
			LoadTextures(object, vulkanEngine, material, aiTextureType_HEIGHT, Assets::TextureType::BUMP, sceneMaterials, loadedTextures);

			newMesh.MaterialName = material == nullptr ? "DefaultMaterial" : material->GetName().C_Str();
			newMesh.MaterialIndex = static_cast<uint32_t>(GetMaterialIndex(sceneMaterials, newMesh.MaterialName));

			return newMesh;
		}

		void ModelLoader::LoadTextures(
			Assets::Object& object, 
			VulkanEngine& vulkanEngine,
			aiMaterial* material, 
			aiTextureType textureType, 
			Assets::TextureType customTextureType,
			std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures
		) {
			
			if (material == nullptr)
				return;

			for (size_t i = 0; i < material->GetTextureCount(textureType); i++) {
				aiString util;

				material->GetTexture(textureType, i, &util);

				ProcessTexture(
					sceneMaterials, 
					loadedTextures, 
					customTextureType, 
					util.C_Str(), 
					object.MaterialPath, 
					material->GetName().C_Str(), 
					vulkanEngine, 
					object.FlipTexturesVertically, 
					object.GenerateMipMaps
				);

				object.Textured = true;
			}
		}

		void ModelLoader::TinyLoad(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures, VulkanEngine& vulkanEngine) {
			if (object.ModelPath == nullptr && object.MaterialPath == nullptr) {
				LoadCustomModel(object, sceneMaterials);
				return;
			}

			std::cout << "Loading model - " << object.ModelPath << '\n';

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

				if (GetMaterialIndex(sceneMaterials, material.name) >= 0)
					continue;

				Assets::Material newMaterial = {};
				newMaterial.Name = material.name;
				newMaterial.MaterialData.Diffuse = { material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f };
				newMaterial.MaterialData.Specular = { material.specular[0], material.specular[1], material.specular[2], 1.0f };
				//newMaterial.MaterialData.Transmittance = { material.transmittance[0], material.transmittance[1], material.transmittance[2], 1.0f };
				newMaterial.MaterialData.Emission = { material.emission[0], material.emission[1], material.emission[2], 1.0f };
				newMaterial.MaterialData.Shininess = material.shininess;
				//newMaterial.MaterialData.Ior = material.ior;
				//newMaterial.MaterialData.Dissolve = material.dissolve;
				newMaterial.MaterialData.Roughness = material.roughness;
				newMaterial.MaterialData.Metallic = material.metallic;
				newMaterial.MaterialData.Sheen = material.sheen;
				newMaterial.MaterialData.ClearcoatThickness = material.clearcoat_thickness;
				newMaterial.MaterialData.ClearcoatRoughness = material.clearcoat_roughness;
				newMaterial.MaterialData.Anisotropy = material.anisotropy;
				newMaterial.MaterialData.AnisotropyRotation = material.anisotropy_rotation;
				//sceneMaterials[material.name]->MaterialData.Pad0 = material.pad0;
				newMaterial.MaterialData.Pad2 = material.pad2;
				newMaterial.MaterialData.Illum = material.illum;

				sceneMaterials.push_back(newMaterial);

				std::map<Assets::TextureType, std::string> textureMap{
					{ Assets::TextureType::AMBIENT, material.ambient_texname },								// 0
					{ Assets::TextureType::DIFFUSE, material.diffuse_texname },								// 1
					{ Assets::TextureType::SPECULAR, material.specular_texname },							// 2
					{ Assets::TextureType::BUMP, material.bump_texname },									// 3
					{ Assets::TextureType::ROUGHNESS, material.roughness_texname },							// 4
					{ Assets::TextureType::METALLIC, material.metallic_texname },							// 5
					{ Assets::TextureType::NORMAL, material.normal_texname }								// 6
					//{ Assets::TextureType::SPECULAR_HIGHTLIGHT, material.specular_highlight_texname },	// 7
					//{ Assets::TextureType::DISPLACEMENT, material.displacement_texname },					// 8
					//{ Assets::TextureType::ALPHA, material.alpha_texname },								// 9
					//{ Assets::TextureType::REFLECTION, material.reflection_texname },						// 10
					//{ Assets::TextureType::SHEEN, material.sheen_texname },								// 11
					//{ Assets::TextureType::EMISSIVE, material.emissive_texname },							// 12
				};

				std::map<Assets::TextureType, std::string>::iterator it;
				for (it = textureMap.begin(); it != textureMap.end(); it++) {
					if (!object.Textured)
						object.Textured = true;

					ModelLoader::ProcessTexture(
						sceneMaterials,
						loadedTextures,
						it->first,
						it->second,
						modelBasePath,
						material.name,
						vulkanEngine,
						object.FlipTexturesVertically,
						object.GenerateMipMaps
					);
				}
			}

			std::cout << "Loading model meshes..." << '\n';

			for (const auto& shape : shapes) {
				std::unordered_map<Assets::Vertex, uint32_t> uniqueVertices{};

				Assets::Mesh newMesh = Assets::Mesh();

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
						uniqueVertices[vertex] = static_cast<uint32_t>(newMesh.Vertices.size());
						newMesh.Vertices.push_back(vertex);
					}

					newMesh.Indices.push_back(uniqueVertices[vertex]);
				}

				newMesh.MaterialName = materials.size() == 0 ? "DefaultMaterial" : materials[shape.mesh.material_ids[0]].name;
				newMesh.MaterialIndex = static_cast<size_t>(GetMaterialIndex(sceneMaterials, newMesh.MaterialName));

				object.Meshes.push_back(newMesh);
			}
		}

		void ModelLoader::ProcessTexture(
			std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures,
			Assets::TextureType textureType,
			std::string textureName,
			std::string basePath,
			std::string materialName,
			VulkanEngine& vulkanEngine,
			bool flipTexturesVertically,
			bool generateMipMaps
		) {
			std::string texName = textureName;
			std::string path = basePath;

			if (textureName == "" || !fileExists(basePath + textureName)) {
				texName = "error_texture.jpg";
				path = "C:/Users/Felipe/Documents/current_projects/VulkanApplication/src/Assets/Textures/";
			}

			ValidateAndInsertTexture(
				loadedTextures, 
				textureType, 
				texName, 
				path, 
				vulkanEngine,
				flipTexturesVertically,
				generateMipMaps
			);
			LoadTextureToMaterial(sceneMaterials, loadedTextures, textureType, texName, materialName);
		}

		void ModelLoader::ValidateAndInsertTexture(
				std::vector<Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				VulkanEngine& vulkanEngine,
				bool flipTexturesVertically,
				bool generateMipMaps
			) {

			int textureIndex = GetTextureIndex(loadedTextures, textureName);

			if (textureIndex != -1)
				return;

			loadedTextures.push_back(
				TextureLoader::LoadTexture(
					(basePath + textureName).c_str(),
					vulkanEngine,
					flipTexturesVertically,
					generateMipMaps
				)
			);

			loadedTextures[loadedTextures.size() - 1].Type = textureType;
			loadedTextures[loadedTextures.size() - 1].Name = textureName;
		}

		void ModelLoader::LoadTextureToMaterial(
			std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures,
			Assets::TextureType textureType,
			std::string textureName,
			std::string materialName
		) {
			int materialIndex = GetMaterialIndex(sceneMaterials, materialName);
			switch (textureType) {
			case Assets::TextureType::AMBIENT:
				sceneMaterials[materialIndex].MaterialData.AmbientTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
				break;
			case Assets::TextureType::DIFFUSE:
				sceneMaterials[materialIndex].MaterialData.DiffuseTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
				break;
			case Assets::TextureType::SPECULAR:
				sceneMaterials[materialIndex].MaterialData.SpecularTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
				break;
			case Assets::TextureType::BUMP:
				sceneMaterials[materialIndex].MaterialData.BumpTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
				break;
			case Assets::TextureType::ROUGHNESS:
				sceneMaterials[materialIndex].MaterialData.RoughnessTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
				break;
			case Assets::TextureType::METALLIC:
				sceneMaterials[materialIndex].MaterialData.MetallicTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
				break;
			case Assets::TextureType::NORMAL:
				sceneMaterials[materialIndex].MaterialData.NormalTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
				break;
			default:
				break;
			};
		}

		int ModelLoader::GetTextureIndex(std::vector<Assets::Texture>& loadedTextures, std::string textureName) {
			if (loadedTextures.size() == 0) 
				return UNEXISTENT;

			for (int i = 0; i < loadedTextures.size(); i++) {
				if (loadedTextures[i].Name == textureName) {
					return i;
				}
			}

			return UNEXISTENT;
		}

		int ModelLoader::GetMaterialIndex(std::vector<Assets::Material>& sceneMaterials, std::string materialName) {
			if (sceneMaterials.size() == 0)
				return UNEXISTENT;

			for (int i = 0; i < sceneMaterials.size(); i++) {
				if (sceneMaterials[i].Name == materialName)
					return i;
			}

			return UNEXISTENT;
		}

		void ModelLoader::LoadCustomModel(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials) {
			for (auto& mesh : object.Meshes) {
				if (GetMaterialIndex(sceneMaterials, mesh.MaterialName) == UNEXISTENT) {
					sceneMaterials.push_back(mesh.CustomMeshMaterial);
				}

				mesh.MaterialIndex = GetMaterialIndex(sceneMaterials, mesh.MaterialName);
			}
		}

		bool ModelLoader::fileExists(const std::string& path) {
			struct stat buffer;
			return (stat(path.c_str(), &buffer) == 0);
		}
	}
}
