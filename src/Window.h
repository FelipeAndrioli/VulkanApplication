#pragma once

#include <iostream>
#include <functional>
#include <stdexcept>

#include "Vulkan.h"
#include "WindowSettings.h"
#include "Time.h"

namespace Engine {
	class Window {
	public:
		Window(WindowSettings *p_WindowSettings);
		~Window();

		void Run();
		bool IsMinimized();
		void WaitEvents();
		void Close();

		inline GLFWwindow* GetHandle() const { return m_Window; };
		VkExtent2D GetFramebufferSize() const;

		// function callbacks
		std::function<void()> DrawFrame;
		std::function<void(int width, int height)> OnResize;
		std::function<void(int key, int scancode, int action, int mods)> OnKeyPress;
	private:
		GLFWwindow* m_Window{};
		WindowSettings* p_WindowSettings;
		bool m_Running;

		std::unique_ptr<class Time> m_Time;
	};
}
