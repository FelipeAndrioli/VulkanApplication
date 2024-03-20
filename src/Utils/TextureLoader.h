#pragma once

#include <memory>
#include <stdexcept>

#include "../Vulkan.h"
#include "../Image.h"

#include "../../Assets/Texture.h"

namespace Engine {
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class Image;

	namespace Utils {
		class TextureLoader {
		public:
			TextureLoader();
			~TextureLoader();

			static Assets::Texture LoadTexture(
				const char* texturePath, 
				LogicalDevice& logicalDevice, 
				PhysicalDevice& physicalDevice,
				CommandPool& commandPool,
				bool flipTextureVertically,
				bool generateMipMaps
			);
		};
	}
}
