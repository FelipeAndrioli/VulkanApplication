#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace Assets {
	class Model;
}

struct Material;

namespace Graphics {
	struct Texture;
}

namespace ModelLoader {
	void CompileMesh(Assets::Model& model, std::vector<Material>& materials);
	std::shared_ptr<Assets::Model> LoadModel(const std::string& path, std::vector<Material>& materials, std::vector<Graphics::Texture>& textures);
}