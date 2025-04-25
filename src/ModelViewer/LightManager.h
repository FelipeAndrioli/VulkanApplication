#pragma once

#include <string>
#include <vector>

#include <glm.hpp>

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/SceneComponents.h"

#include "../Core/VulkanHeader.h"
#include "../Core/ConstantBuffers.h"

#define MAX_LIGHTS 5

namespace LightManager {
	extern Graphics::Buffer m_LightBuffer;

	void Init();
	void Shutdown();

	void AddLight(Scene::LightComponent& light);
	void UpdateBuffer();
	void OnUIRender();

	int GetTotalLights();

	Graphics::Buffer& GetLightBuffer();

	std::vector<Scene::LightComponent>& GetLights();
}
