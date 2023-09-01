#include "DescriptorPool.h"

namespace Engine {
	DescriptorPool::DescriptorPool(VkDevice& logicalDevice, std::vector<PoolDescriptorBinding> descriptorBindings, uint32_t maxSets) : p_LogicalDevice(logicalDevice) {
		std::vector<VkDescriptorPoolSize> poolSizes;

		for (auto descriptorBinding : descriptorBindings) {
			VkDescriptorPoolSize newDescriptor = {};
			newDescriptor.type = descriptorBinding.type;
			newDescriptor.descriptorCount = static_cast<uint32_t>(descriptorBinding.descriptorCount);

			poolSizes.push_back(newDescriptor);
		}

		/*
		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

			We need to double the number of VK_DESCRIPTOR_TYPE_STORAGE_BUFFER types requested from the pool
			because our sets reference the SSBOs of the last and current frame (for now).
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
		*/

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;

		if (vkCreateDescriptorPool(p_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Descriptor Pool!");
		}
	}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(p_LogicalDevice, m_DescriptorPool, nullptr);
	}
}