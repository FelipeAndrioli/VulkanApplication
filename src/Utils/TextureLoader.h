#pragma once

#include <memory>
#include <stdexcept>

#include "../GraphicsDevice.h"
#include "../Graphics.h"

using namespace Engine::Graphics;

namespace TextureLoader {
	static Texture LoadTexture(const char* texturePath, Texture::TextureType textureType, bool flipTextureVertically, bool generateMipMaps);
	static Texture LoadCubemapTexture(const char* texturePath);
	static Texture LoadCubemapTexture(std::vector<std::string> texturePaths);
}
