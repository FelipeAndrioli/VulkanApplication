#include "Material.h"

#include "../src/Image.h"
#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"

#include "Texture.h"

namespace Assets {
	Material::Material() {

	}

	Material::~Material() {
		//Textures.clear();
		DescriptorSets.reset();
	}
}