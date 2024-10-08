#pragma once

#include <memory>
#include <stdexcept>

#include "../Core/GraphicsDevice.h"
#include "../Core/Graphics.h"

using namespace Graphics;

namespace TextureLoader {
	extern Texture LoadTexture(const char* texturePath, Texture::TextureType textureType, bool flipTextureVertically, bool generateMipMaps);
	extern Texture LoadCubemapTexture(const char* texturePath);
	extern Texture LoadCubemapTexture(std::vector<std::string> texturePaths);
}
