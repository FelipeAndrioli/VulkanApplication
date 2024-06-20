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

		for (const auto& descriptorBinding : descriptorSetLayout.GetDescriptorBindings()) {
			if (descriptorBinding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && descriptorBinding.buffer) {
				WriteDescriptorUniformBuffer(logicalDevice, m_DescriptorSet, descriptorBinding);
			}

			if (descriptorBinding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && descriptorBinding.textures) {
				WriteDescriptorImages(logicalDevice, m_DescriptorSet, descriptorBinding);
			}

			if (descriptorBinding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && descriptorBinding.texture) {
				WriteDescriptorImage(logicalDevice, m_DescriptorSet, descriptorBinding);
			}
		}
	}
	
	DescriptorSets::~DescriptorSets() {
	
	}

	void DescriptorSets::WriteDescriptorUniformBuffer(const VkDevice& logicalDevice, const VkDescriptorSet& descriptorSet, 
		const DescriptorBinding& descriptorBinding) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = descriptorBinding.buffer->GetHandle();
		bufferInfo.offset = descriptorBinding.bufferOffset;
		bufferInfo.range = descriptorBinding.bufferSize == 0 ? 256 : descriptorBinding.bufferSize;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = descriptorBinding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorBinding.type;
		descriptorWrite.descriptorCount = descriptorBinding.descriptorCount;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}

	void DescriptorSets::WriteDescriptorImages(const VkDevice& logicalDevice, const VkDescriptorSet& descriptorSet, const DescriptorBinding& descriptorBinding) {

		if (descriptorBinding.textures->empty() || descriptorBinding.textures->size() == 0)
			return;

		std::vector<VkDescriptorImageInfo> imageInfo;
		VkWriteDescriptorSet descriptorWrite = {};

		for (auto& texture : *descriptorBinding.textures) {
			VkDescriptorImageInfo newImageInfo = {};
			newImageInfo.imageLayout = texture.TextureImage->ImageLayout;
			newImageInfo.imageView = texture.TextureImage->ImageView;
			newImageInfo.sampler = texture.TextureImage->ImageSampler;

			imageInfo.push_back(newImageInfo);
		}

		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = descriptorBinding.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorBinding.type;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfo.size());
		descriptorWrite.pImageInfo = imageInfo.data();
		
		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr); 
	}

	void DescriptorSets::WriteDescriptorImage(const VkDevice& logicalDevice, const VkDescriptorSet& descriptorSet, const DescriptorBinding& descriptorBinding) {

		VkDescriptorImageInfo newImageInfo = {};
		newImageInfo.imageLayout = descriptorBinding.texture->TextureImage->ImageLayout;
		newImageInfo.imageView = descriptorBinding.texture->TextureImage->ImageView;
		newImageInfo.sampler = descriptorBinding.texture->TextureImage->ImageSampler;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
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
