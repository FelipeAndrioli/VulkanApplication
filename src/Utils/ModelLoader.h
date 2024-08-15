#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../Assets/Model.h"
#include "../Assets/Material.h"
#include "../Graphics.h"

namespace Renderer {

	void CompileMesh(Model& model, std::vector<Assets::Material>& materials);
	std::shared_ptr<Model> LoadModel(const std::string& path, std::vector<Assets::Material>& materials, std::vector<Engine::Graphics::Texture>& textures);
}