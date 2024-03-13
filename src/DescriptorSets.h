#pragma once

#include <stdexcept>
#include <vector>
#include <memory>

#include "Vulkan.h"
//temporary
#include "Common.h"
#include "DescriptorPool.h"
#include "Buffer.h"
#include "Image.h"

namespace Assets {
	enum TextureType;
	struct Texture;
};

namespace Engine {
	class DescriptorSetLayout;

	class DescriptorSets {
	public:
		DescriptorSets() {};
		DescriptorSets(
			const VkDevice& logicalDevice,
			const VkDescriptorPool& descriptorPool,
			DescriptorSetLayout& descriptorSetLayout
		);

		~DescriptorSets();
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
	private:
		std::vector<VkDescriptorSet> m_DescriptorSets;
	};
}
