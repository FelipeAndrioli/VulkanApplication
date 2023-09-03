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

	Window::Window(WindowSettings *windowSettings) : p_WindowSettings(windowSettings) {

		if (!glfwInit()) {
			throw std::runtime_error("Failed to initialize window!");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(p_WindowSettings->Width, p_WindowSettings->Height, 
			p_WindowSettings->Title.c_str(), nullptr, nullptr);

		glfwSetWindowUserPointer(m_Window, this);
		
		glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
		glfwSetKeyCallback(m_Window, keyCallback);

		m_Running = true;

		m_Time.reset(new class Time());
	}

	Window::~Window() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void Window::Run() {
		while (!glfwWindowShouldClose(m_Window) && m_Running) {
			glfwPollEvents();
			
			p_WindowSettings->ms = m_Time->GetElapsedTime();

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

	void Window::WaitEvents() {
		glfwWaitEvents();
	}

	VkExtent2D Window::GetFramebufferSize() const {
		int width;
		int height;

		glfwGetFramebufferSize(m_Window, &width, &height);

		return VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	}
}
