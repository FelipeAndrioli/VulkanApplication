#pragma once

#include <GLFW/glfw3.h>

namespace Engine {
	namespace InputSystem {
		struct key_t {
			bool IsPressed = false;
			bool IsDown = false;
		};

		struct mouse_t {
			double X = 0.0;
			double Y = 0.0;

			double LastX = 0.0;
			double LastY = 0.0;

			int OnScreen = 0;

			bool FirstMouse = true;
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
