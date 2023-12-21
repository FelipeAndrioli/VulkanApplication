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

	void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (_this->OnMouseClick) {
			_this->OnMouseClick(button, action, mods);
		}
	}

	void cursorPosCallback(GLFWwindow* window, double x, double y) {
		auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (_this->OnCursorMove) {
			_this->OnCursorMove(x, y);
		}
	}

	void cursorEnterCallback(GLFWwindow* window, int entered) {
		auto* const _this = static_cast<Window*>(glfwGetWindowUserPointer(window));
	
		if (_this->OnCursorOnScreen) {
			_this->OnCursorOnScreen(entered);
		}
	}

	Window::Window(Settings& settings) : p_Settings(&settings) {

		if (!glfwInit()) {
			throw std::runtime_error("Failed to initialize window!");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(p_Settings->Width, p_Settings->Height, 
			p_Settings->Title.c_str(), nullptr, nullptr);

		glfwSetWindowUserPointer(m_Window, this);
		
		glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
		glfwSetKeyCallback(m_Window, keyCallback);
		glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);
		glfwSetCursorPosCallback(m_Window, cursorPosCallback);
		glfwSetCursorEnterCallback(m_Window, cursorEnterCallback);

		m_Running = true;
	}

	Window::~Window() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void Window::Run() {
		while (!glfwWindowShouldClose(m_Window) && m_Running) {
			glfwPollEvents();

			m_CurrentFrameTime = (float)glfwGetTime();
			Timestep timestep = m_CurrentFrameTime - m_LastFrameTime;

			if (p_Settings->limitFramerate && timestep.GetSeconds() < (1 / 60.0f)) continue;
			
			if (Update) {
				Update(timestep.GetMilliseconds());
			}
			
			if (Render) {
				Render();
			}

			m_LastFrameTime = m_CurrentFrameTime;

			p_Settings->ms = timestep.GetMilliseconds();
			p_Settings->frames = static_cast<float>(1 / timestep.GetSeconds());
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
