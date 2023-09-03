#pragma once

#include "Vulkan.h"

namespace Engine {
	class Time {
	public:
		Time();
		~Time();

		double GetElapsedTime();
	};
}