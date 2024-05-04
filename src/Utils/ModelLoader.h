#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>

#include "../Assets/Object.h"
#include "../Assets/Mesh.h"
#include "../Assets/Texture.h"
#include "../Assets/Material.h"

namespace Engine {
	/*
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	*/

	class VulkanEngine;

	namespace Utils {
		class ModelLoader {
		public:
			ModelLoader();
			~ModelLoader();

			static void LoadModelAndMaterials(
				Assets::Object& object, 
				std::vector<Assets::Material>& sceneMaterials,				
				std::vector<Assets::Texture>& loadedTextures,
				VulkanEngine& vulkanEngine
			);

			static void LoadCustomModel(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials);

		private:

			static void ProcessNode(
				Assets::Object& object,
				VulkanEngine& vulkanEngine,
				const aiNode* node, 
				const aiScene* scene, 
				std::vector<Assets::Material>& sceneMaterials,				
				std::vector<Assets::Texture>& loadedTextures
			);

			static Assets::Mesh ProcessMesh(
				Assets::Object& object,
				VulkanEngine& vulkanEngine,
				const aiMesh* mesh, 
				const aiScene* scene,
				std::vector<Assets::Material>& sceneMaterials,				
				std::vector<Assets::Texture>& loadedTextures
			);

			static void LoadTextures(
				Assets::Object& object,
				VulkanEngine& vulkanEngine,
				aiMaterial* material, 
				aiTextureType textureType, 
				Assets::TextureType customTextureType,
				std::vector<Assets::Material>& sceneMaterials,				
				std::vector<Assets::Texture>& loadedTextures
			);

			static void TinyLoad(
				Assets::Object& object,
				std::vector<Assets::Material>& sceneMaterials,
				std::vector<Assets::Texture>& loadedTextures,
				VulkanEngine& vulkanEngine
			);

			static void ProcessTexture(
				std::vector<Assets::Material>& sceneMaterials,
				std::vector<Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				std::string materialName,
				VulkanEngine& vulkanEngine,
				bool flipTexturesVertically,
				bool generateMipMaps
			);

			static void ValidateAndInsertTexture(
				std::vector<Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				VulkanEngine& vulkanEngine,
				bool flipTexturesVertically,
				bool generateMipMaps
			);

			static void LoadTextureToMaterial(
				std::vector<Assets::Material>& sceneMaterials,
				std::vector<Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string materialName
			);

			static int GetTextureIndex(std::vector<Assets::Texture>& loadedTextures, std::string textureName);
			static int GetMaterialIndex(std::vector<Assets::Material>& sceneMaterials, std::string materialName);

			static bool fileExists(const std::string& path);
		};
	}
}
