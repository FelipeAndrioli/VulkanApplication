#include "Time.h"

namespace Engine {
	Time::Time() {
		
	}

	Time::~Time() {

	}

	double Time::GetElapsedTime() {
		return glfwGetTime();
	}
}