#pragma once

#include <stdexcept>
#include <vector>
#include <memory>

#include "Vulkan.h"
//temporary
#include "Common.h"
#include "DescriptorPool.h"
#include "DescriptorBinding.h"
#include "Buffer.h"

namespace Engine {
	class DescriptorSets {
	public:
		DescriptorSets(
			const VkDeviceSize& bufferSize, 
			const VkDevice& logicalDevice, 
			const VkDescriptorPool& descriptorPool, 
			const VkDescriptorSetLayout& descriptorSetLayout, 
			Buffer* uniformBuffers,
			Buffer* shaderStorageBuffers = nullptr,
			bool accessLastFrame = false
		);

		~DescriptorSets();
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
	private:
		std::vector<VkDescriptorSet> m_DescriptorSets;
	};
}
