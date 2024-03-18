#pragma once

#include <array>
#include <vector>
#include <memory>
#include <stdexcept>

#include "Vulkan.h"
#include "DescriptorBinding.h"
#include "DescriptorPool.h"

namespace Engine {
	class DescriptorPool {
	public:
		DescriptorPool(VkDevice& logicalDevice, std::vector<PoolDescriptorBinding> descriptorBindings, uint32_t maxSets);
		~DescriptorPool();

		inline VkDescriptorPool& GetHandle() { return m_DescriptorPool; };
	private:
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

		VkDevice& m_LogicalDevice;
	};

	class DescriptorPoolBuilder {
	public:
		DescriptorPoolBuilder() {};
		~DescriptorPoolBuilder() {};

		DescriptorPoolBuilder& AddDescriptorType(VkDescriptorType descriptorType);
		DescriptorPoolBuilder& AddDescriptorCount(uint32_t descriptorCount);
		DescriptorPoolBuilder& AddBinding();
		DescriptorPoolBuilder& SetMaxSets(uint32_t maxSets);
		std::unique_ptr<class DescriptorPool> Build(VkDevice& logicalDevice);

		std::vector<PoolDescriptorBinding> m_Bindings;
		VkDescriptorType m_DescriptorType;
		uint32_t m_DescriptorCount = 1;
		uint32_t m_MaxSets = 1;
	};
}
