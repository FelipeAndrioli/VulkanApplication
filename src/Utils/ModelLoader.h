#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

#include "../Graphics.h"
#include "../GraphicsDevice.h"

using namespace Engine::Graphics;

namespace ModelLoader {
	static void LoadModelAndMaterials(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials, std::vector<Texture>& loadedTextures);
	static void LoadCustomModel(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials);
}
