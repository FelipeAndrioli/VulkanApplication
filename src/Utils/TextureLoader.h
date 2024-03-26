#pragma once

#include <memory>
#include <stdexcept>

#include "../Vulkan.h"
#include "../Image.h"

#include "../../Assets/Texture.h"

namespace Engine {
	class VulkanEngine;
	class Image;

	namespace Utils {
		class TextureLoader {
		public:
			TextureLoader();
			~TextureLoader();

			static Assets::Texture LoadTexture(
				const char* texturePath, 
				VulkanEngine& vulkanEngine,
				bool flipTextureVertically,
				bool generateMipMaps
			);
		};
	}
}
