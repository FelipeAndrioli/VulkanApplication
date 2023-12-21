#pragma once

#include <memory>
#include <stdexcept>

#include "../Vulkan.h"

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
				std::unique_ptr<Image>& texture,
				const char* texturePath, 
				LogicalDevice& logicalDevice, 
				PhysicalDevice& physicalDevice,
				CommandPool& commandPool
			);
		};
	}
}
