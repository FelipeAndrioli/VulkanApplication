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

			if (object.ModelPath == nullptr && object.MaterialPath == nullptr) {
				LoadCustomModel(object, sceneMaterials);
				return;
			}

			const aiScene* scene = aiImportFile(object.ModelPath, aiProcess_Triangulate);

			if (!scene || !scene->HasMeshes()) {
				throw std::runtime_error("Unable to load file!");
			}

			ProcessNode(object, scene->mRootNode, scene);
			ProcessMaterials(vulkanEngine, object, scene, sceneMaterials, loadedTextures);

			LinkMeshesToMaterials(object.Meshes, sceneMaterials);
		}

		void ModelLoader::ProcessNode(Assets::Object& object, const aiNode* node, const aiScene* scene) {
			for (size_t i = 0; i < node->mNumMeshes; i++) {
				const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				//object.Meshes.push_back(ProcessMesh(mesh, scene));
				object.AddMesh(ProcessMesh(mesh, scene));
			}

			for (size_t i = 0; i < node->mNumChildren; i++) {
				ProcessNode(object, node->mChildren[i], scene);
			}
		}

		Assets::Mesh ModelLoader::ProcessMesh(const aiMesh* mesh, const aiScene* scene) {
			std::vector<Assets::Vertex> vertices;
			std::vector<uint32_t> indices;
			Assets::Mesh newMesh = {};
			std::unordered_map<Assets::Vertex, uint32_t> uniqueVertices = {};

			for (size_t i = 0; i < mesh->mNumFaces; i++) {

				const aiFace face = mesh->mFaces[i];

				for (size_t j = 0; j < face.mNumIndices; j++) {
					Assets::Vertex vertex = {};
					vertex.pos = {
						mesh->mVertices[face.mIndices[j]].x,
						mesh->mVertices[face.mIndices[j]].y,
						mesh->mVertices[face.mIndices[j]].z
					};

					if (mesh->mTextureCoords[0]) {
						vertex.texCoord = {
							mesh->mTextureCoords[0][face.mIndices[j]].x,
							mesh->mTextureCoords[0][face.mIndices[j]].y
						};
					}

					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<uint32_t>(newMesh.Vertices.size());
						newMesh.Vertices.push_back(vertex);
					}

					newMesh.Indices.push_back(uniqueVertices[vertex]);
				}
			}

			newMesh.MaterialName = scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
			
			return newMesh;
		}

		void ModelLoader::ProcessMaterials(
			VulkanEngine& vulkanEngine,
			Assets::Object& object,
			const aiScene* scene, 
			std::vector<Assets::Material>& sceneMaterials,
			std::vector<Assets::Texture>& loadedTextures) {

			for (size_t i = 0; i < scene->mNumMaterials; i++) {
				aiMaterial* material = scene->mMaterials[i];

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
			}
		}

		void ModelLoader::LinkMeshesToMaterials(std::vector<Assets::Mesh>& meshes, std::vector<Assets::Material>& sceneMaterials) {
			for (auto& mesh : meshes) {
				if (GetMaterialIndex(sceneMaterials, mesh.MaterialName) == UNEXISTENT) {
					sceneMaterials.push_back(mesh.CustomMeshMaterial);
					mesh.MaterialName = "Default";
				} 

				mesh.MaterialIndex = GetMaterialIndex(sceneMaterials, mesh.MaterialName);
			}
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
				std::cout << "File: " << basePath + textureName << " doesn't exists! Loading custom texture!" << '\n';
				texName = "error_texture.jpg";
				path = "./Textures/";
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
			LinkMeshesToMaterials(object.Meshes, sceneMaterials);
			object.Textured = false;
		}

		bool ModelLoader::fileExists(const std::string& path) {
			struct stat buffer;
			return (stat(path.c_str(), &buffer) == 0);
		}
	}
}
