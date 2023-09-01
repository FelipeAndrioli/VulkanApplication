#pragma once

#include "Vulkan.h"

namespace Engine {
	struct DescriptorBinding {
		uint32_t binding;
		uint32_t descriptorCount;
		VkDescriptorType type;
		VkShaderStageFlags stage;
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
