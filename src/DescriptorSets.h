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
		VkDescriptorSet& GetHandle() { return m_DescriptorSet; }
		void WriteDescriptorUniformBuffer(const VkDevice& logicalDevice, const DescriptorBinding& descriptorBinding, 
			const VkBuffer& buffer, const size_t bufferSize, const size_t bufferOffset);
		void WriteDescriptorImages(const VkDevice& logicalDevice, const DescriptorBinding& descriptorBinding, std::vector<Assets::Texture>& textures);
		void WriteDescriptorImage(const VkDevice& logicalDevice, const DescriptorBinding& descriptorBinding, Assets::Texture& texture);
		void Bind(const VkCommandBuffer& commandBuffer, const VkPipelineBindPoint& bindPoint, const VkPipelineLayout& pipelineLayout);
	private:
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		uint32_t m_Set = 0;
		uint32_t m_SetCount = 0;
	};
}
