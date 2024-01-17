#include "Material.h"

#include "../src/Image.h"

#include "Texture.h"

namespace Assets {
	Material::Material() {

	}

	Material::~Material() {

		for (Assets::Texture& texture : Textures) {
			texture.TextureImage.reset();
		}

		Textures.clear();
	}
}