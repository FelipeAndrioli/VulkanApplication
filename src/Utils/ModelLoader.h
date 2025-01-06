#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <glm.hpp>

namespace Assets {
	class Model;
}

struct Material;

namespace Graphics {
	struct Texture;
}

enum ModelType {
	CUBE,
	QUAD,
	PLANE
};

namespace ModelLoader {
	std::shared_ptr<Assets::Model> LoadModel(const std::string& path);
	std::shared_ptr<Assets::Model> LoadModel(ModelType modelType, glm::vec3 position = glm::vec3(0.0), float size = 1.0f);
}