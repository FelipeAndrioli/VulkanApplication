#pragma once

#include <vector>
#include <stdexcept>

#include "Vulkan.h"

namespace Engine {
	class Semaphore {
	public:
		Semaphore(const VkDevice &device, const int size);
		~Semaphore();

		VkSemaphore* GetHandle(const int index);

	private:
		VkDevice r_Device;
		std::vector<VkSemaphore> m_Semaphores;
		int m_SemaphoresSize;
	};
}
