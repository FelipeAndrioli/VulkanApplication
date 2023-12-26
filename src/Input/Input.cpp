#include "./Input.h"

namespace Engine {
	namespace InputSystem {
		Input::Input() {

		}

		Input::~Input() {

		}

		void Input::ProcessKey(int key, int scancode, int action, int mods) {
			if (key < 0) return;

			/*
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
				std::cout << "Closing application" << '\n';
				m_Window->Close();
			}
			*/

			switch (action) {
			case GLFW_PRESS:
				Keys[key].IsDown = true;
				Keys[key].IsPressed = true;
				break;
			case GLFW_RELEASE:
				Keys[key].IsDown = false;
				Keys[key].IsPressed = false;
				break;
			default:
				break;
			}
		}

		void Input::ProcessMouseClick(int button, int action, int mods) {
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
				Mouse.LeftButtonPressed = true;
			}
			
			if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
				Mouse.RightButtonPressed = true;
			}

			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
				Mouse.LeftButtonPressed = false;
			}
			
			if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
				Mouse.RightButtonPressed = false;
			}
		}

		void Input::ProcessCursorMove(double x, double y) {
			Mouse.x = x;
			Mouse.y = y;
		}

		void Input::ProcessCursorOnScreen(int entered) {
			if (entered == 1) Mouse.OnScreen = true;
			if (entered == 0) Mouse.OnScreen = false;
		}
	}
}