#include "Material.h"

#include "Buffer.h"
#include "BufferHelper.h"
#include "PipelineLayout.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"
#include "SwapChain.h"
#include "DepthBuffer.h"
#include "Image.h"
#include "./Utils/TextureLoader.h"

#include "../../Assets/Texture.h"

namespace Engine {
	Material::Material() {

	}

	Material::~Material() {

		for (Assets::Texture& texture : Textures) {
			texture.TextureImage.reset();
		}

		Textures.clear();
	}
}