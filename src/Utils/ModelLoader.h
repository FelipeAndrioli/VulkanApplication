#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

namespace Assets {
	class Object;
	struct Material;
	struct Texture;

	enum TextureType;
}

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
				/*
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool
				*/
			);

			static void LoadCustomModel(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials);

			static inline bool fileExists(const std::string& path) {
				struct stat buffer;
				return (stat(path.c_str(), &buffer) == 0);
			}

		private:
			static void ProcessTexture(
				std::vector<Assets::Material>& sceneMaterials,
				std::vector<Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				std::string materialName,
				VulkanEngine& vulkanEngine,
				/*
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool,
				*/
				bool flipTexturesVertically,
				bool generateMipMaps
			);

			static void ValidateAndInsertTexture(
				std::vector<Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				VulkanEngine& vulkanEngine,
				/*
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool,
				*/
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
		};
	}
}
