#include "Material.h"

#include "../src/Image.h"
#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"

#include "Texture.h"

namespace Assets {
	Material::Material() {

	}

	Material::~Material() {

		std::map<Assets::TextureType, Assets::Texture>::iterator it;
		for (it = Textures.begin(); it != Textures.end(); it++) {
			it->second.TextureImage.reset();
		}

		Textures.clear();

		GPUDataBuffer.reset();
		DescriptorSets.reset();
	}
}