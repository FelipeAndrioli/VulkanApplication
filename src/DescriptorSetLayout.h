#pragma once

#include <stdexcept>
#include <vector>

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
			size_t bufferSize, 
			size_t bufferOffset
		);
	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;
		std::vector<DescriptorBinding> m_DescriptorBindings;
		VkDevice& p_LogicalDevice;
	};
}
