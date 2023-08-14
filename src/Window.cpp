#include "Window.h"

namespace Engine {

	void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
		auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (_this->OnResize) {
			_this->OnResize(width, height);
		}
	}

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (_this->OnKeyPress) {
			_this->OnKeyPress(key, scancode, action, mods);
		}
	}

	Window::Window(const WindowSettings &r_WindowSettings) : m_WindowSettings(r_WindowSettings) {

		if (!glfwInit()) {
			throw std::runtime_error("Failed to initialize window!");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(m_WindowSettings.Width, m_WindowSettings.Height, 
			m_WindowSettings.Title.c_str(), nullptr, nullptr);

		glfwSetWindowUserPointer(m_Window, this);
		
		glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
		glfwSetKeyCallback(m_Window, keyCallback);

		m_Running = true;
	}

	Window::~Window() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void Window::Run() {
		while (!glfwWindowShouldClose(m_Window) && m_Running) {
			glfwPollEvents();
			
			if (DrawFrame) {
				DrawFrame();
			}
		}
	}

	void Window::Close() {
		m_Running = false;
		glfwSetWindowShouldClose(m_Window, 1);
	}

	bool Window::IsMinimized() {
		const auto framebufferSize = GetFramebufferSize();

		return framebufferSize.width == 0 && framebufferSize.height == 0;
	}

	GLFWwindow* Window::GetHandle() const {
		return m_Window;
	}

	VkExtent2D Window::GetFramebufferSize() const {
		int width;
		int height;

		glfwGetFramebufferSize(m_Window, &width, &height);

		return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	}
}
