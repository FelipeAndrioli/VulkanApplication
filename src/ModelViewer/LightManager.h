#pragma once

#include "../Core/Graphics.h"
#include "../Core/SceneComponents.h"

#define MAX_SPOT_LIGHTS 2
#define MAX_DIR_LIGHTS 1
#define MAX_POINT_LIGHTS 2
#define MAX_LIGHT_SOURCES (MAX_SPOT_LIGHTS + MAX_DIR_LIGHTS + MAX_POINT_LIGHTS)

namespace Assets {
	class ShadowCamera;
};

class LightManager {
public:
	LightManager() = default;
	~LightManager() {};

	LightManager(const LightManager& other) = delete;
	LightManager(LightManager&& other) = delete;
	LightManager& operator=(const LightManager& other) = delete;
	LightManager& operator=(LightManager&& other) = delete;

	void Init();
	void Shutdown();
	Scene::LightComponent* GetLight(uint32_t index);
	uint32_t AddLight(const Scene::LightComponent& light, uint32_t flags = (1 << 4 | 1 << 1));
	void RemoveLight(uint32_t index);
	void Update(Assets::ShadowCamera& camera);
	void OnUIRender();
	void ChangeLightType(uint32_t index, Scene::LightComponent::LightType type);

	Graphics::Buffer LightBuffer;

	Scene::LightComponent Lights[MAX_LIGHT_SOURCES];
	
	uint32_t NextExternalIndex = 0;
	uint32_t ExternalToInternal[MAX_LIGHT_SOURCES];
	uint32_t InternalToExternal[MAX_LIGHT_SOURCES];
	
	uint32_t		LightCount		[Scene::LightComponent::LightType::TYPE_COUNT];
	const uint32_t	LightOffsets	[Scene::LightComponent::LightType::TYPE_COUNT] = { 0, MAX_DIR_LIGHTS, (MAX_DIR_LIGHTS + MAX_POINT_LIGHTS) };
	const uint32_t	LightLimits		[Scene::LightComponent::LightType::TYPE_COUNT] = { MAX_DIR_LIGHTS, MAX_POINT_LIGHTS, MAX_SPOT_LIGHTS };

	uint32_t TotalLights = MAX_LIGHT_SOURCES;

	int LightShadowRenderDebugIndex = -1;

private:
	uint32_t FindNextSuitableExternalPosition();
};
