#include "Fence.h"

namespace Engine {
	Fence::Fence(const VkDevice& device, const int size) {
		r_Device = device;
		m_FencesSize = size;
		m_Fences.resize(m_FencesSize);

		for (size_t i = 0; i < m_FencesSize; i++) {
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateFence(r_Device, &fenceInfo, nullptr, &m_Fences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create fences!");
			}
		}
	}

	Fence::~Fence() {
		for (size_t i = 0; i < m_FencesSize; i++) {
			vkDestroyFence(r_Device, m_Fences[i], nullptr);
		}
	}

	VkFence* Fence::GetHandle(const int index) {
		if (index > m_FencesSize) {
			throw std::runtime_error("Fence index out of bounds!");
		}

		return &m_Fences[index];
	}
}