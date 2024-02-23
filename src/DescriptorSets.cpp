#include "DescriptorSets.h"

#include "../Assets/Texture.h"

namespace Engine {
	DescriptorSets::DescriptorSets(
		const VkDeviceSize& bufferSize, 
		const VkDevice& logicalDevice, 
		const VkDescriptorPool& descriptorPool, 
		const VkDescriptorSetLayout& descriptorSetLayout, 
		Buffer* uniformBuffers, 
		std::map<Assets::TextureType, Assets::Texture*>* textures,
		Buffer* shaderStorageBuffers,
		bool accessLastFrame,
		VkDeviceSize offset
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

		// TODO: refactor descriptor set creation
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers->GetBuffer(static_cast<uint32_t>(i));
			bufferInfo.offset = offset;
			bufferInfo.range = bufferSize;

			if (!accessLastFrame) {
				std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

				int index = 0;
	
				descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[index].dstSet = m_DescriptorSets[i];
				descriptorWrites[index].dstBinding = 0;
				descriptorWrites[index].dstArrayElement = 0;
				descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[index].descriptorCount = 1;
				descriptorWrites[index].pBufferInfo = &bufferInfo;
				descriptorWrites[index].pImageInfo = nullptr;
				descriptorWrites[index].pTexelBufferView = nullptr;

				index++;

				if (textures && textures->size() > 0) {
					VkDescriptorImageInfo imageInfo[TEXTURE_PER_MATERIAL];

					std::map<Assets::TextureType, Assets::Texture*>::iterator it;
					int t = 0;

					for (it = textures->begin(); it != textures->end(); it++) {
						imageInfo[t].imageLayout = it->second->TextureImage->ImageLayout;
						imageInfo[t].imageView = it->second->TextureImage->ImageView[0];
						imageInfo[t].sampler = it->second->TextureImage->ImageSampler;

						t++;
					}

					descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[index].dstSet = m_DescriptorSets[i];
					descriptorWrites[index].dstBinding = 1;
					descriptorWrites[index].dstArrayElement = 0;
					descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[index].descriptorCount = TEXTURE_PER_MATERIAL;
					descriptorWrites[index].pImageInfo = imageInfo;

					index++;
				}

				vkUpdateDescriptorSets(logicalDevice, index, 
					descriptorWrites.data(), 0, nullptr);
			} else {

				int index = 0;

				std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};
				descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[index].dstSet = m_DescriptorSets[i];
				descriptorWrites[index].dstBinding = 0;
				descriptorWrites[index].dstArrayElement = 0;
				descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[index].descriptorCount = 1;
				descriptorWrites[index].pBufferInfo = &bufferInfo;
				descriptorWrites[index].pImageInfo = nullptr;
				descriptorWrites[index].pTexelBufferView = nullptr;

				index++;	

				VkDescriptorBufferInfo storageBufferInfoLastFrame = {};
				storageBufferInfoLastFrame.buffer = shaderStorageBuffers->GetBuffer((i - 1) % MAX_FRAMES_IN_FLIGHT);
				storageBufferInfoLastFrame.offset = 0;
				storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

				descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[index].dstSet = m_DescriptorSets[i];
				descriptorWrites[index].dstBinding = 2;
				descriptorWrites[index].dstArrayElement = 0;
				descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[index].descriptorCount = 1;
				descriptorWrites[index].pBufferInfo = &storageBufferInfoLastFrame;
				descriptorWrites[index].pImageInfo = nullptr;
				descriptorWrites[index].pTexelBufferView = nullptr;

				index++;

				VkDescriptorBufferInfo storageBufferInfoCurrentFrame = {};
				storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers->GetBuffer((i - 1) % MAX_FRAMES_IN_FLIGHT);
				storageBufferInfoCurrentFrame.offset = 0;
				storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

				descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[index].dstSet = m_DescriptorSets[i];
				descriptorWrites[index].dstBinding = 3;
				descriptorWrites[index].dstArrayElement = 0;
				descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrites[index].descriptorCount = 1;
				descriptorWrites[index].pBufferInfo = &storageBufferInfoCurrentFrame;
				descriptorWrites[index].pImageInfo = nullptr;
				descriptorWrites[index].pTexelBufferView = nullptr;

				index++;

				vkUpdateDescriptorSets(logicalDevice, index, descriptorWrites.data(),
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