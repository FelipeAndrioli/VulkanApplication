#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <memory>

namespace Assets {
	class Object;
	class Material;
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
				Engine::LogicalDevice& logicalDevice,
				Engine::PhysicalDevice& physicalDevice,
				Engine::CommandPool& commandPool
			);
		};
	}
}
