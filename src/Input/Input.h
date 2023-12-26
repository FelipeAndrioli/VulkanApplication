#pragma once

#include <GLFW/glfw3.h>

namespace Engine {
	namespace InputSystem {
		struct key_t {
			bool IsPressed = false;
			bool IsDown = false;
		};

		struct mouse_t {
			double x = 0.0;
			double y = 0.0;

			bool LeftButtonPressed = false;
			bool RightButtonPressed = false;
			bool OnScreen = true;
		};

		class Input {
		public:
			Input();
			~Input();

			void ProcessKey(int key, int scancode, int action, int mods);
			void ProcessMouseClick(int button, int action, int mods);
			void ProcessCursorMove(double x, double y);
			void ProcessCursorOnScreen(int entered);

		public:
			key_t Keys[GLFW_KEY_LAST];
			mouse_t Mouse;
		};
	}
}
