#pragma once

#include <string>
#include <vector>

#include <glm.hpp>

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/SceneComponents.h"

#include "../Core/VulkanHeader.h"
#include "../Core/ConstantBuffers.h"

#define MAX_LIGHT_SOURCES 5

namespace Assets {
	class ShadowCamera;
};

namespace LightManager {
	extern Graphics::Buffer m_LightBuffer;

	void Init			();
	void Shutdown		();
	void AddLight		(Scene::LightComponent& light);
	void DeleteLight	(Scene::LightComponent& light);
	void DeleteLight	(int lightIndex);
	void UpdateBuffer	();
	void Update			(Assets::ShadowCamera& camera);
	void OnUIRender		();
	
	int GetTotalLights	();
	int GetLightShadowDebugIndex();

	Graphics::Buffer& GetLightBuffer();

	std::vector<Scene::LightComponent>& GetLights();
}
