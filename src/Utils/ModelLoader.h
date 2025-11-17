#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <glm.hpp>

namespace Assets {
	class Model;
}

enum ModelType {
	CUBE,
	QUAD,
	PLANE
};

namespace {
	static std::unordered_map<std::string, int> loadedFileNames;

}

namespace ModelLoader {

	void FlipModelUvVertically(Assets::Model& model);

	std::shared_ptr<Assets::Model> LoadModel(const std::string& path);
	std::shared_ptr<Assets::Model> LoadModel(ModelType modelType, glm::vec3 position = glm::vec3(0.0), float size = 1.0f);
	std::shared_ptr<Assets::Model> DuplicateModel(std::shared_ptr<Assets::Model> model);
}