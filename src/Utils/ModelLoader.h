#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>

#include "../Graphics.h"
#include "../GraphicsDevice.h"
#include "../Assets/Object.h"

using namespace Engine::Graphics;

namespace ModelLoader {
	extern void LoadModelAndMaterials(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials, std::vector<Texture>& loadedTextures);
	extern void LoadCustomModel(Assets::Object& object, std::vector<Assets::Material>& sceneMaterials);
}
