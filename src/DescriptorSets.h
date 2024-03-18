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
			DescriptorSetLayout& descriptorSetLayout,
			const uint32_t set,
			const uint32_t setCount
		);

		~DescriptorSets();
		VkDescriptorSet& GetDescriptorSet(uint32_t index);
		void WriteDescriptorUniformBuffer(const VkDevice& logicalDevice, const VkDescriptorSet& descriptorSet, const DescriptorBinding& descriptorBinding, const size_t bufferIndex);
		void WriteDescriptorImage(const VkDevice& logicalDevice, const VkDescriptorSet& descriptorSet, const DescriptorBinding& descriptorBinding);
		void Bind(
			const uint32_t setIndex,
			const VkCommandBuffer& commandBuffer, 
			const VkPipelineBindPoint& bindPoint,
			const VkPipelineLayout& pipelineLayout
		);
	private:
		std::vector<VkDescriptorSet> m_DescriptorSets;
		uint32_t m_Set = 0;
		uint32_t m_SetCount = 0;
	};
}
