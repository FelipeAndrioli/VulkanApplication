#pragma once

#include <stdexcept>

#include "VulkanHeader.h"

namespace Engine {
	class Surface {
	public:
		Surface(VkInstance& instance, GLFWwindow& window);
		~Surface();

		inline VkSurfaceKHR& GetHandle() { return m_Surface; };
	private:
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		VkInstance& p_Instance;
		GLFWwindow& p_Window;
	};
}
