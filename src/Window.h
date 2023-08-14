#pragma once

#include <iostream>
#include <functional>
#include <stdexcept>

#include "Vulkan.h"
#include "WindowSettings.h"

namespace Engine {
	class Window {
	public:
		Window(const WindowSettings &r_WindowSettings);
		~Window();

		void Run();
		bool IsMinimized();
		void Close();

		GLFWwindow* GetHandle() const;
		VkExtent2D GetFramebufferSize() const;

		// function callbacks
		std::function<void()> DrawFrame;
		std::function<void(int width, int height)> OnResize;
		std::function<void(int key, int scancode, int action, int mods)> OnKeyPress;
	private:
		GLFWwindow* m_Window{};
		const WindowSettings m_WindowSettings;
		bool m_Running;
	};
}
