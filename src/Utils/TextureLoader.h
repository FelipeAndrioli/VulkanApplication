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

			static void LoadTexture(
				std::unique_ptr<class Image>& texture,
				const char* texturePath, 
				LogicalDevice& logicalDevice, 
				PhysicalDevice& physicalDevice,
				CommandPool& commandPool,
				bool flipTextureVertically
			);

			static Assets::Texture CreateTexture(
				Assets::TextureType textureType,
				const char* texturePath,
				LogicalDevice& logicalDevice,
				PhysicalDevice& physicalDevice,
				CommandPool& commandPool,
				bool flipTextureVertically
			);
		};
	}
}
