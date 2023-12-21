#pragma once

#include <string>

namespace Engine {
	struct Settings {
		std::string Title = "VulkanApplication.exe";
		uint32_t Width = 800;
		uint32_t Height = 600;
		float ms = 0.0f;
		float frames = 0.0f;
		bool limitFramerate = false;
	};
}
