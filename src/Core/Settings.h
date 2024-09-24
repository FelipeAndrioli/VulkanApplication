#pragma once

#include <string>

struct Settings {
	std::string Title = "VulkanApplication.exe";
	uint32_t Width = 800;
	uint32_t Height = 600;
	bool uiEnabled = false;
	bool renderWireframe = false;
	bool renderSkybox = false;
	bool renderDefault = true;
	bool renderLightSources = true;
};
