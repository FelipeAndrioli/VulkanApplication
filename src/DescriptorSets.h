#pragma once

#include <stdexcept>
#include <map>
#include <memory>

#include "Vulkan.h"
//temporary
#include "Common.h"
#include "DescriptorPool.h"
#include "DescriptorBinding.h"
#include "Buffer.h"
#include "Image.h"

namespace Assets {
	enum TextureType;
	struct Texture;
};

namespace Engine {
	class DescriptorSets {
	public:
		DescriptorSets(
			const VkDeviceSize& bufferSize, 
			const VkDevice& logicalDevice, 
			const VkDescriptorPool& descriptorPool, 
			const VkDescriptorSetLayout& descriptorSetLayout, 
			Buffer* uniformBuffers,
			std::map<Assets::TextureType, Assets::Texture*>* textures,
			Buffer* shaderStorageBuffers = nullptr,
			bool accessLastFrame = false,
			VkDeviceSize offset = 0
		);

		~DescriptorSets();
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
	private:
		std::vector<VkDescriptorSet> m_DescriptorSets;
	};
}
