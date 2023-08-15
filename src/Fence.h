#pragma once

#include <vector>
#include <stdexcept>
#include "Vulkan.h"

namespace Engine {
	class Fence {
	public:
		Fence(const VkDevice &device, const int size);
		~Fence();

		VkFence* GetHandle(int index);

	private:
		VkDevice r_Device;
		std::vector<VkFence> m_Fences;
		int m_FencesSize;
	};
}
