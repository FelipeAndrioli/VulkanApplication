#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <memory>

namespace Assets {
	class Object;
	class Material;
	struct Texture;

	enum TextureType;
}

namespace Engine {
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;

	namespace Utils {
		class ModelLoader {
		public:
			ModelLoader();
			~ModelLoader();

			static void LoadModelAndMaterials(
				Assets::Object& object, 
				std::map<std::string, std::unique_ptr<Assets::Material>>& sceneMaterials,
				std::map<std::string, Assets::Texture>& loadedTextures,
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool
			);

			static inline bool fileExists(const std::string& path) {
				struct stat buffer;
				return (stat(path.c_str(), &buffer) == 0);
			}

		private:
			static void ProcessTexture(
				std::map<std::string, std::unique_ptr<Assets::Material>>& sceneMaterials,
				std::map<std::string, Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				std::string materialName,
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool,
				bool flipTexturesVertically
			);

			static void ValidateAndInsertTexture(
				std::map<std::string, Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string basePath,
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool,
				bool flipTexturesVertically
			);

			static void LoadTextureToMaterial(
				std::map<std::string, std::unique_ptr<Assets::Material>>& sceneMaterials,
				std::map<std::string, Assets::Texture>& loadedTextures,
				Assets::TextureType textureType,
				std::string textureName,
				std::string materialName
			);
		};
	}
}
