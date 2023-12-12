#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

#include "../Vulkan.h"
#include "../Buffer.h"
#include "../BufferHelper.h"
#include "../LogicalDevice.h"
#include "../PhysicalDevice.h"
#include "../CommandPool.h"
#include "../Image.h"

namespace Engine {
	namespace Utils {
		class TextureLoader {
		public:
			TextureLoader();
			~TextureLoader();

			void LoadTexture(
				const char* texturePath, 
				LogicalDevice& logicalDevice, 
				PhysicalDevice& physicalDevice,
				CommandPool& commandPool	
			);
		};
	}
}
