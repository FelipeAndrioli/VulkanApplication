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
		DescriptorSets(VkDevice& logicalDevice, VkDescriptorPool& descriptorPool, 
			VkDescriptorSetLayout descriptorSetLayout, Buffer* uniformBuffers,
			bool accessLastFrame = false, Buffer* shaderStorageBuffers = nullptr);
		~DescriptorSets();
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
	private:
		std::vector<VkDescriptorSet> m_DescriptorSets;

		VkDevice& p_LogicalDevice;
		VkDescriptorPool& p_DescriptorPool;
		VkDescriptorSetLayout& p_DescriptorSetLayout;
		Buffer* p_UniformBuffers;
		Buffer* p_ShaderStorageBuffers;
	};
}
