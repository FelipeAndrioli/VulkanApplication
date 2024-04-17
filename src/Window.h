#pragma once

#include <iostream>
#include <functional>
#include <stdexcept>

#include "Vulkan.h"
#include "Settings.h"
#include "Timestep.h"

namespace Engine {
	class Window {
	public:
		Window(Settings& settings);
		~Window();

		void Run();
		bool IsMinimized();
		void WaitEvents();
		void Close();

		inline float GetLastFrameTime() { return m_LastFrameTime; }
		float GetCurrentFrametime() { return m_CurrentFrameTime; }
		inline GLFWwindow* GetHandle() const { return m_Window; }
		VkExtent2D GetFramebufferSize() const;

		std::function<void(float time)> Update;
		std::function<void()> Render;
		std::function<void(int width, int height)> OnResize;
		std::function<void(int key, int scancode, int action, int mods)> OnKeyPress;
		std::function<void(int button, int action, int mods)> OnMouseClick;
		std::function<void(double x, double y)> OnCursorMove;
		std::function<void(int entered)> OnCursorOnScreen;
	private:
		GLFWwindow* m_Window{};
		Settings* p_Settings = nullptr;
		bool m_Running;

		float m_CurrentFrameTime = 0.0;
		float m_LastFrameTime = 0.0;
	};
}
