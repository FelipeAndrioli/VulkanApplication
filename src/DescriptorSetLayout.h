#pragma once

#include <stdexcept>
#include <vector>
#include <memory>

#include "Vulkan.h"
#include "DescriptorBinding.h"

namespace Assets {
	struct Texture;
}

namespace Engine {
	class Buffer;
	class DescriptorSetLayout {
	public:
		DescriptorSetLayout(std::vector<DescriptorBinding>& descriptorBindings, VkDevice& logicalDevice);
		~DescriptorSetLayout();

		inline VkDescriptorSetLayout& GetHandle() { return m_DescriptorSetLayout; };
		std::vector<DescriptorBinding>& GetDescriptorBindings() { return m_DescriptorBindings; }

		void Update(
			const size_t index, 
			Buffer* buffer, 
			std::vector<Assets::Texture>* textures, 
			Assets::Texture* texture, 
			size_t bufferSize, 
			size_t bufferOffset
		);
		
		void UpdateOffset(size_t index, size_t bufferOffset) { m_DescriptorBindings[index].bufferOffset = bufferOffset; }
	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;
		std::vector<DescriptorBinding> m_DescriptorBindings;
		VkDevice& p_LogicalDevice;
	};

	class DescriptorSetLayoutBuild {
	public:
		DescriptorSetLayoutBuild() {};
		~DescriptorSetLayoutBuild() {};

		DescriptorSetLayoutBuild& NewBinding(uint32_t binding);
		DescriptorSetLayoutBuild& SetDescriptorCount(uint32_t descriptorCount);
		DescriptorSetLayoutBuild& SetType(VkDescriptorType type);
		DescriptorSetLayoutBuild& SetStage(VkShaderStageFlags stage);
		DescriptorSetLayoutBuild& SetResource(Buffer& buffer);
		DescriptorSetLayoutBuild& SetResource(std::vector<Assets::Texture>& textures);
		DescriptorSetLayoutBuild& SetResource(Assets::Texture& texture);
		DescriptorSetLayoutBuild& SetBufferSize(VkDeviceSize bufferSize);
		DescriptorSetLayoutBuild& SetBufferOffset(VkDeviceSize bufferOffset);
		DescriptorSetLayoutBuild& Add();
		std::unique_ptr<class DescriptorSetLayout> Build(VkDevice& logicalDevice);

	private:
		std::vector<DescriptorBinding> m_DescriptorBindings;
		DescriptorBinding m_DescriptorBinding;
	};
}
