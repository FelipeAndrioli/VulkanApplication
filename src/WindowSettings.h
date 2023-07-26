#pragma once

#include <string>

namespace Engine {
	struct WindowSettings {
		std::string Title = "VulkanApplication.exe";
		uint32_t Width = 800;
		uint32_t Height = 600;
	};
}
