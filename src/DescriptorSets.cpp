#include "DescriptorSets.h"
#include "DescriptorSetLayout.h"

#include "./Assets/Texture.h"

namespace Engine {
	DescriptorSets::DescriptorSets(
		const VkDevice& logicalDevice, 
		const VkDescriptorPool& descriptorPool, 
		DescriptorSetLayout& descriptorSetLayout,
		const uint32_t set,
		const uint32_t setCount
	) : m_Set(set), m_SetCount(setCount) {
		std::vector<VkDescriptorSetLayout> layouts(1, descriptorSetLayout.GetHandle());

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Descriptor Sets!");
		}
	}
	
	DescriptorSets::~DescriptorSets() {
	
	}

	void DescriptorSets::WriteDescriptorUniformBuffer(const VkDevice& logicalDevice, const DescriptorBinding& descriptorBinding, 
		const VkBuffer& buffer, const size_t bufferSize, const size_t bufferOffset) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = buffer;
		bufferInfo.offset = bufferOffset;
		bufferInfo.range = bufferSize == 0 ? 256 : bufferSize;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = descriptorBinding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorBinding.type;
		descriptorWrite.descriptorCount = descriptorBinding.descriptorCount;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}

	void DescriptorSets::WriteDescriptorImages(const VkDevice& logicalDevice, const DescriptorBinding& descriptorBinding, std::vector<Assets::Texture>& textures) {

		if (textures.size() == 0)
			return;

		std::vector<VkDescriptorImageInfo> imageInfo;
		VkWriteDescriptorSet descriptorWrite = {};

		for (auto& texture : textures) {
			VkDescriptorImageInfo newImageInfo = {};
			newImageInfo.imageLayout = texture.TextureImage->ImageLayout;
			newImageInfo.imageView = texture.TextureImage->ImageView;
			newImageInfo.sampler = texture.TextureImage->ImageSampler;

			imageInfo.push_back(newImageInfo);
		}

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = descriptorBinding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorBinding.type;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfo.size());
		descriptorWrite.pImageInfo = imageInfo.data();
		
		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr); 
	}

	void DescriptorSets::WriteDescriptorImage(const VkDevice& logicalDevice, const DescriptorBinding& descriptorBinding, Assets::Texture& texture) {

		VkDescriptorImageInfo newImageInfo = {};
		newImageInfo.imageLayout = texture.TextureImage->ImageLayout;
		newImageInfo.imageView = texture.TextureImage->ImageView;
		newImageInfo.sampler = texture.TextureImage->ImageSampler;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = descriptorBinding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorBinding.type;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &newImageInfo;
		
		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr); 
	}

	void DescriptorSets::Bind(const VkCommandBuffer& commandBuffer, const VkPipelineBindPoint& bindPoint, const VkPipelineLayout& pipelineLayout) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, m_Set, m_SetCount, &m_DescriptorSet, 0, nullptr);
	}
}
