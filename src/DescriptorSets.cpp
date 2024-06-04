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
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout.GetHandle());

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Descriptor Sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			for (const auto& descriptorBinding : descriptorSetLayout.GetDescriptorBindings()) {
				if (descriptorBinding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && descriptorBinding.buffer) {
					WriteDescriptorUniformBuffer(logicalDevice, m_DescriptorSets[i], descriptorBinding, i);
				}

				if (descriptorBinding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && descriptorBinding.textures) {
					WriteDescriptorImages(logicalDevice, m_DescriptorSets[i], descriptorBinding);
				}

				if (descriptorBinding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && descriptorBinding.texture) {
					WriteDescriptorImage(logicalDevice, m_DescriptorSets[i], descriptorBinding);
				}
			}
		}
	}
	
	DescriptorSets::~DescriptorSets() {
	
	}

	VkDescriptorSet& DescriptorSets::GetDescriptorSet(uint32_t index) {
		if (index > m_DescriptorSets.size() || index < 0) {
			throw std::runtime_error("Index out of bounds trying to access the descriptor set!");
		}

		return m_DescriptorSets[index];
	}

	void DescriptorSets::WriteDescriptorUniformBuffer(
		const VkDevice& logicalDevice, 
		const VkDescriptorSet& descriptorSet, 
		const DescriptorBinding& descriptorBinding, 
		const size_t bufferIndex
	) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = descriptorBinding.buffer->GetBuffer(static_cast<uint32_t>(bufferIndex));
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

		vkUpdateDescriptorSets(
			logicalDevice,
			1,
			&descriptorWrite,
			0,
			nullptr
		);
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
		
		vkUpdateDescriptorSets(
			logicalDevice,
			1,
			&descriptorWrite,
			0,
			nullptr
		); 
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
		
		vkUpdateDescriptorSets(
			logicalDevice,
			1,
			&descriptorWrite,
			0,
			nullptr
		); 
	}

	void DescriptorSets::Bind(
		const uint32_t setIndex,
		const VkCommandBuffer& commandBuffer, 
		const VkPipelineBindPoint& bindPoint,
		const VkPipelineLayout& pipelineLayout
	) {
		vkCmdBindDescriptorSets(
			commandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			pipelineLayout,
			m_Set, 
			m_SetCount,
			&m_DescriptorSets[setIndex],
			0, 
			nullptr
		);
	}
}
