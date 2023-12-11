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

		public:
			key_t Keys[GLFW_KEY_LAST];
			mouse_t Mouse;
		};
	}
}
