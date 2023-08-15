#include "Semaphore.h"

namespace Engine {
	Semaphore::Semaphore(const VkDevice &device, const int size) {
		r_Device = device;
		m_SemaphoresSize = size;
		m_Semaphores.resize(m_SemaphoresSize);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (size_t i = 0; i < m_SemaphoresSize; i++) {
			if (vkCreateSemaphore(r_Device, &semaphoreInfo, nullptr, &m_Semaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create semaphores!");
			}
		}
	}

	Semaphore::~Semaphore() {
		for (size_t i = 0; i < m_SemaphoresSize; i++) {
			vkDestroySemaphore(r_Device, m_Semaphores[i], nullptr);
		}
	}

	VkSemaphore* Semaphore::GetHandle(const int index) {
		if (index > m_SemaphoresSize) {
			throw std::runtime_error("Out of bounds sempahore!");
		}

		return &m_Semaphores[index];
	}
}