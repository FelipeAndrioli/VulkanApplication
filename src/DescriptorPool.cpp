#include "DescriptorPool.h"

namespace Engine {
	DescriptorPool::DescriptorPool(VkDevice& logicalDevice, std::vector<PoolDescriptorBinding> descriptorBindings, uint32_t maxSets) : m_LogicalDevice(logicalDevice) {
		std::vector<VkDescriptorPoolSize> poolSizes;

		for (auto descriptorBinding : descriptorBindings) {
			for (size_t i = 0; i < maxSets; i++) {
				VkDescriptorPoolSize newDescriptor = {};
				newDescriptor.type = descriptorBinding.type;
				newDescriptor.descriptorCount = static_cast<uint32_t>(descriptorBinding.descriptorCount);

				poolSizes.push_back(newDescriptor);
			}
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;

		if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Descriptor Pool!");
		}
	}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
	}

	DescriptorPoolBuilder& DescriptorPoolBuilder::AddDescriptorType(VkDescriptorType descriptorType) {
		m_DescriptorType = descriptorType;
		return *this;
	}

	DescriptorPoolBuilder& DescriptorPoolBuilder::AddDescriptorCount(uint32_t descriptorCount) {
		m_DescriptorCount = descriptorCount;
		return *this;
	}

	DescriptorPoolBuilder& DescriptorPoolBuilder::AddBinding() {
		PoolDescriptorBinding binding = { m_DescriptorType, m_DescriptorCount };
		m_Bindings.push_back(binding);

		return *this;
	}

	DescriptorPoolBuilder& DescriptorPoolBuilder::SetMaxSets(uint32_t maxSets) {
		m_MaxSets = maxSets;

		return *this;
	}


	std::unique_ptr<class DescriptorPool> DescriptorPoolBuilder::Build(VkDevice& logicalDevice) {
		std::unique_ptr<class DescriptorPool> descriptorPool = std::make_unique<class DescriptorPool>(logicalDevice, m_Bindings, m_MaxSets);

		m_Bindings.clear();

		return descriptorPool;
	}
}