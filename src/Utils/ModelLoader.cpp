#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <unordered_map>
#include <stdexcept>

#include <sys/stat.h>

#include <assimp/mesh.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "../../Assets/Object.h"
#include "../../Assets/Mesh.h"
#include "../../Assets/Material.h"

#include "./TextureLoader.h"

#include <algorithm>
#include <execution>

constexpr auto UNEXISTENT = -1;

namespace ModelLoader {
	
	int GetTextureIndex(std::vector<Texture>& loadedTextures, std::string textureName) {
		if (loadedTextures.size() == 0) 
			return UNEXISTENT;

		for (int i = 0; i < loadedTextures.size(); i++) {
			if (loadedTextures[i].Name == textureName) {
				return i;
			}
		}

		return UNEXISTENT;
	}

	int GetMaterialIndex(std::vector<Assets::Material>& sceneMaterials, std::string materialName) {
		if (sceneMaterials.size() == 0)
			return UNEXISTENT;

		for (int i = 0; i < sceneMaterials.size(); i++) {
			if (sceneMaterials[i].Name == materialName)
				return i;
		}

		return UNEXISTENT;
	}

	bool fileExists(const std::string& path) {
		struct stat buffer;
		return (stat(path.c_str(), &buffer) == 0);
	}

	Assets::Mesh ProcessMesh(const aiMesh* mesh, const aiScene* scene) {
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

	void ProcessNode(Assets::Object& object, const aiNode* node, const aiScene* scene) {
		for (size_t i = 0; i < node->mNumMeshes; i++) {
			const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//object.Meshes.push_back(ProcessMesh(mesh, scene));
			object.AddMesh(ProcessMesh(mesh, scene));
		}

		for (size_t i = 0; i < node->mNumChildren; i++) {
			ProcessNode(object, node->mChildren[i], scene);
		}
	}

	void LinkMeshesToMaterials(std::vector<Assets::Mesh>& meshes, std::vector<Assets::Material>& sceneMaterials) {
		for (auto& mesh : meshes) {
			if (GetMaterialIndex(sceneMaterials, mesh.MaterialName) == UNEXISTENT) {
				sceneMaterials.push_back(mesh.CustomMeshMaterial);
				mesh.MaterialName = "Default";
			} 

			mesh.MaterialIndex = GetMaterialIndex(sceneMaterials, mesh.MaterialName);
		}
	}

	void ValidateAndInsertTexture(
			std::vector<Texture>& loadedTextures,
			Texture::TextureType textureType,
			std::string textureName,
			std::string basePath,
			bool flipTexturesVertically,
			bool generateMipMaps
		) {

		int textureIndex = GetTextureIndex(loadedTextures, textureName);

		if (textureIndex != -1)
			return;

		loadedTextures.push_back(
			TextureLoader::LoadTexture(
				(basePath + textureName).c_str(),
				textureType,
				flipTexturesVertically,
				generateMipMaps
			)
		);

		loadedTextures[loadedTextures.size() - 1].Name = textureName;
	}

	void LoadTextureToMaterial(
		std::vector<Assets::Material>& sceneMaterials,
		std::vector<Texture>& loadedTextures,
		Texture::TextureType textureType,
		std::string textureName,
		std::string materialName
	) {
		int materialIndex = GetMaterialIndex(sceneMaterials, materialName);
		switch (textureType) {
		case Texture::TextureType::AMBIENT:
			sceneMaterials[materialIndex].MaterialData.AmbientTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
			break;
		case Texture::TextureType::DIFFUSE:
			sceneMaterials[materialIndex].MaterialData.DiffuseTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
			break;
		case Texture::TextureType::SPECULAR:
			sceneMaterials[materialIndex].MaterialData.SpecularTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
			break;
		case Texture::TextureType::BUMP:
			sceneMaterials[materialIndex].MaterialData.BumpTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
			break;
		case Texture::TextureType::ROUGHNESS:
			sceneMaterials[materialIndex].MaterialData.RoughnessTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
			break;
		case Texture::TextureType::METALLIC:
			sceneMaterials[materialIndex].MaterialData.MetallicTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
			break;
		case Texture::TextureType::NORMAL:
			sceneMaterials[materialIndex].MaterialData.NormalTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
			break;
		default:
			break;
		};
	}

	void LoadCustomModel(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials) {
		LinkMeshesToMaterials(object.Meshes, sceneMaterials);
		object.Textured = false;
	}

	void ProcessTexture(
		std::vector<Assets::Material>& sceneMaterials,
		std::vector<Texture>& loadedTextures,
		Texture::TextureType textureType,
		std::string textureName,
		std::string basePath,
		std::string materialName,
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
			flipTexturesVertically,
			generateMipMaps
		);
		LoadTextureToMaterial(sceneMaterials, loadedTextures, textureType, texName, materialName);
	}

	void LoadTextures(
		Assets::Object& object, 
		aiMaterial* material, 
		aiTextureType textureType, 
		Texture::TextureType customTextureType,
		std::vector<Assets::Material>& sceneMaterials,
		std::vector<Texture>& loadedTextures
	) {
		
		if (material == nullptr)
			return;

		for (uint32_t i = 0; i < material->GetTextureCount(textureType); i++) {
			aiString util;

			material->GetTexture(textureType, i, &util);

			ProcessTexture(
				sceneMaterials, 
				loadedTextures, 
				customTextureType, 
				util.C_Str(), 
				object.MaterialPath, 
				material->GetName().C_Str(), 
				object.FlipTexturesVertically, 
				object.GenerateMipMaps
			);

			object.Textured = true;
		}
	}

	static void ProcessMaterials(
		Assets::Object& object,
		const aiScene* scene, 
		std::vector<Assets::Material>& sceneMaterials,
		std::vector<Texture>& loadedTextures) {

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

			LoadTextures(object, material, aiTextureType_AMBIENT, Texture::TextureType::AMBIENT, sceneMaterials, loadedTextures);
			LoadTextures(object, material, aiTextureType_DIFFUSE, Texture::TextureType::DIFFUSE, sceneMaterials, loadedTextures);
			LoadTextures(object, material, aiTextureType_SPECULAR, Texture::TextureType::SPECULAR, sceneMaterials, loadedTextures);
			LoadTextures(object, material, aiTextureType_NORMALS, Texture::TextureType::NORMAL, sceneMaterials, loadedTextures);
			LoadTextures(object, material, aiTextureType_HEIGHT, Texture::TextureType::BUMP, sceneMaterials, loadedTextures);
		}
	}

	void LoadModelAndMaterials(
		Assets::Object& object, 
		std::vector<Assets::Material>& sceneMaterials,
		std::vector<Texture>& loadedTextures) {

		if (object.ModelPath == nullptr && object.MaterialPath == nullptr) {
			LoadCustomModel(object, sceneMaterials);
			return;
		}

		const aiScene* scene = aiImportFile(object.ModelPath, aiProcess_Triangulate);

		if (!scene || !scene->HasMeshes()) {
			throw std::runtime_error("Unable to load file!");
		}

		ProcessNode(object, scene->mRootNode, scene);
		ProcessMaterials(object, scene, sceneMaterials, loadedTextures);

		LinkMeshesToMaterials(object.Meshes, sceneMaterials);
	}
}
