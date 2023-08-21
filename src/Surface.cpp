#include "Surface.h"

namespace Engine {
	Surface::Surface(VkInstance& instance, GLFWwindow& window) : p_Instance(instance), p_Window(window) {
		if (glfwCreateWindowSurface(p_Instance, &p_Window, nullptr, &m_Surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	Surface::~Surface() {
		vkDestroySurfaceKHR(p_Instance, m_Surface, nullptr);
	}
}