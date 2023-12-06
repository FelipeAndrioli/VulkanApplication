#include "DescriptorSets.h"

namespace Engine {
	DescriptorSets::DescriptorSets(
		VkDeviceSize& bufferSize, 
		VkDevice& logicalDevice, 
		VkDescriptorPool& descriptorPool, 
		VkDescriptorSetLayout& descriptorSetLayout, 
		Buffer* uniformBuffers, 
		Buffer* shaderStorageBuffers,
		bool accessLastFrame 
	) {
		// create descriptor pool inside graphics pipeline
		// receive descriptor set layout from graphics pipeline
		// create this descriptor sets object inside the graphics pipeline class
		// figure out the uniform buffers
		// implement the destructor

		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

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
			VkDescriptorBufferInfo bufferInfo{};
			//bufferInfo.buffer = uniformBuffers.GetBuffer(static_cast<uint32_t>(i));
			bufferInfo.buffer = uniformBuffers->GetBuffer(static_cast<uint32_t>(i));
			bufferInfo.offset = 0;
			bufferInfo.range = bufferSize;

			if (!accessLastFrame) {
				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = m_DescriptorSets[i];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrite.descriptorCount = 1;
				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr;
				descriptorWrite.pTexelBufferView = nullptr;

				vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
			} else {
				std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};
				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_DescriptorSets[i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &bufferInfo;
				descriptorWrites[0].pImageInfo = nullptr;
				descriptorWrites[0].pTexelBufferView = nullptr;

				VkDescriptorBufferInfo storageBufferInfoLastFrame = {};
				storageBufferInfoLastFrame.buffer = shaderStorageBuffers->GetBuffer((i - 1) % MAX_FRAMES_IN_FLIGHT);
				storageBufferInfoLastFrame.offset = 0;
				storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_DescriptorSets[i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;
				descriptorWrites[1].pImageInfo = nullptr;
				descriptorWrites[1].pTexelBufferView = nullptr;

				VkDescriptorBufferInfo storageBufferInfoCurrentFrame = {};
				storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers->GetBuffer((i - 1) % MAX_FRAMES_IN_FLIGHT);
				storageBufferInfoCurrentFrame.offset = 0;
				storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = m_DescriptorSets[i];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;
				descriptorWrites[2].pImageInfo = nullptr;
				descriptorWrites[2].pTexelBufferView = nullptr;

				vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
					0, nullptr);
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
}