#pragma once

#include "Vulkan.h"

namespace Assets {
	struct Texture;
}

namespace Engine {
	class Buffer;

	struct DescriptorBinding {
		uint32_t binding;
		uint32_t descriptorCount;
		VkDescriptorType type;
		VkShaderStageFlags stage;
		Buffer* buffer = nullptr;
		std::vector<Assets::Texture>* textures = nullptr;
		VkDeviceSize bufferSize = 0;
		VkDeviceSize bufferOffset = 0;
	};

	struct PoolDescriptorBinding {
		VkDescriptorType type;
		uint32_t descriptorCount;
	};

	struct BufferDescriptor {
		VkBuffer buffer;
		VkDeviceSize offset;
		VkDeviceSize range;
	};
}
