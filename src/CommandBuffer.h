#pragma once

#include <vector>
#include <stdexcept>

#include "Vulkan.h"

namespace Engine {
	class CommandBuffer {
	public:
		CommandBuffer(uint32_t size, VkCommandPool& commandPool, VkDevice& logicalDevice);
		~CommandBuffer();

		VkCommandBuffer& GetCommandBuffer(uint32_t index);
		VkCommandBuffer& Begin(uint32_t index);
		void End(uint32_t index);
		void End(const VkCommandBuffer& commandBuffer);

		static VkCommandBuffer BeginSingleTimeCommandBuffer(VkDevice& logicalDevice, VkCommandPool& commandPool);
		static void EndSingleTimeCommandBuffer(
			VkDevice& logicalDevice, 
			VkQueue& queue, 
			VkCommandBuffer& commandBuffer, 
			VkCommandPool& commandPool
		);
	private:
		void ValidateIndex(uint32_t index);
	private:
		std::vector<VkCommandBuffer> m_CommandBuffers;

		VkCommandPool& p_CommandPool;
		VkDevice& p_LogicalDevice;
	};
}
