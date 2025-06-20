#include "LightManager.h"

#include "../Core/UI.h"
#include "../Core/ConstantBuffers.h"

#include "../Assets/ShadowCamera.h"

#include "./Renderer.h"

void LightManager::Init() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	LightBuffer = gfxDevice->CreateBuffer(sizeof(Scene::LightComponent) * MAX_LIGHT_SOURCES);

	memset(LightCount, 0, Scene::LightComponent::LightType::TYPE_COUNT * sizeof(uint32_t));
	memset(ExternalToInternal, UINT32_MAX, MAX_LIGHT_SOURCES * sizeof(uint32_t));
	memset(InternalToExternal, UINT32_MAX, MAX_LIGHT_SOURCES * sizeof(uint32_t));
}

void LightManager::Shutdown() {
	LightShadowRenderDebugIndex = -1;

	memset(LightCount, 0, Scene::LightComponent::LightType::TYPE_COUNT * sizeof(uint32_t));
	memset(ExternalToInternal, UINT32_MAX, MAX_LIGHT_SOURCES * sizeof(uint32_t));
	memset(InternalToExternal, UINT32_MAX, MAX_LIGHT_SOURCES * sizeof(uint32_t));
}

Scene::LightComponent* LightManager::GetLight(uint32_t index) {
	if (index >= NextExternalIndex)
		return nullptr;

	uint32_t internalIndex = ExternalToInternal[index];

	if (internalIndex >= MAX_LIGHT_SOURCES || !(Lights[internalIndex].IsActive()))
		return nullptr;

	return &Lights[internalIndex];
}

uint32_t LightManager::FindNextSuitableExternalPosition() {
	for (uint32_t i = 0; i < TotalLights; i++) {
		if (ExternalToInternal[i] == UINT32_MAX)
			return i;
	}

	return UINT32_MAX;
}

uint32_t LightManager::AddLight(const Scene::LightComponent& light, uint32_t flags) {

	if (LightCount[light.type] >= LightLimits[light.type])
		return UINT32_MAX;

	uint32_t externalIndex = UINT32_MAX;

	if (NextExternalIndex + 1 >= TotalLights) {
		NextExternalIndex = FindNextSuitableExternalPosition();

		if (NextExternalIndex == UINT32_MAX)
			return UINT32_MAX;

		externalIndex = NextExternalIndex;
		NextExternalIndex = UINT32_MAX;
	}
	else {
		externalIndex = NextExternalIndex++;
	}

	uint32_t internalIndex = LightOffsets[light.type] + LightCount[light.type]++;

	Lights[internalIndex] = light;
	Lights[internalIndex].flags = flags;
	Lights[internalIndex].index = internalIndex;

	ExternalToInternal[externalIndex] = internalIndex;
	InternalToExternal[internalIndex] = externalIndex;

	return externalIndex;
}

void LightManager::RemoveLight(uint32_t index) {

	if (index >= NextExternalIndex)
		return;

	uint32_t internalIndex = ExternalToInternal[index];

	if (!(Lights[internalIndex].IsActive()))
		return;

	Scene::LightComponent::LightType type = Lights[internalIndex].type;
	uint32_t lastInternalIndex = LightOffsets[type] + LightCount[type] - 1;
	uint32_t lastExternalIndex = InternalToExternal[lastInternalIndex];

	Lights[internalIndex] = Lights[lastInternalIndex];
	Lights[internalIndex].index = internalIndex;

	ExternalToInternal[lastExternalIndex] = internalIndex;
	InternalToExternal[internalIndex] = lastExternalIndex;

	Lights[lastInternalIndex].flags &= ~(1 << 4);

	ExternalToInternal[index] = UINT32_MAX;
	InternalToExternal[lastInternalIndex] = UINT32_MAX;

	LightCount[type]--;
}

void LightManager::ChangeLightType(uint32_t index, Scene::LightComponent::LightType type) {

	if (index > TotalLights || LightCount[type] + 1 > LightLimits[type])
		return;

	uint32_t internalIndex = ExternalToInternal[index];

	if (!Lights[internalIndex].IsActive())
		return;

	uint32_t newInternalIndex = LightOffsets[type] + LightCount[type]++;

	Lights[newInternalIndex] = Lights[internalIndex];
	Lights[newInternalIndex].index = newInternalIndex;
	Lights[newInternalIndex].type = type;

	LightCount[Lights[internalIndex].type]--;
	Lights[internalIndex].flags &= ~(1 << 4);

	ExternalToInternal[index] = newInternalIndex;
	InternalToExternal[newInternalIndex] = index;
	InternalToExternal[internalIndex] = UINT32_MAX;
}

void LightManager::Update(Assets::ShadowCamera& camera) {
	for (uint32_t i = 0; i < TotalLights; i++) {

		Scene::LightComponent& light = Lights[i];

		if (!light.IsActive())
			continue;

		// the light source cube has hardcoded vertices in vertex shader around 
		// the origin (0, 0, 0), therefore the pivot vector can also be hardcoded
		// to the origin.

		glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(light.scale));
		glm::mat4 toPosition = glm::translate(glm::mat4(1.0f), glm::vec3(light.position));
		light.model = toPosition * scale * toOrigin;

		light.cutOffAngle = glm::cos(glm::radians(light.rawCutOffAngle));
		light.outerCutOffAngle = glm::cos(glm::radians(light.rawOuterCutOffAngle));	

		if (light.type == Scene::LightComponent::LightType::DIRECTIONAL) {
			camera.UpdateDirectionalLightShadowMatrix(light.direction);
			light.viewProj = camera.GetShadowMatrix();
		}

		if (light.type == Scene::LightComponent::LightType::SPOT) {
			camera.UpdateSpotLightShadowMatrix(light.position, light.direction);
			light.viewProj = camera.GetShadowMatrix();
		}


		if (light.type == Scene::LightComponent::LightType::POINT) {
			light.viewProj = glm::mat4(1.0f);
		}
	}

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->UpdateBuffer(LightBuffer, Lights);
}

void LightManager::OnUIRender() {
	ImGui::SeparatorText("Light Manager");

	if (ImGui::Button("Add Light")) {
		Scene::LightComponent light = {};
		light.type					= Scene::LightComponent::LightType::POINT;
		light.position				= glm::vec4(0.0f);
		light.direction				= glm::vec4(0.0f);
		light.color					= glm::vec4(1.0f);
		light.ambient				= 0.0f;
		light.diffuse				= 0.5f;
		light.specular				= 0.5f;
		light.linearAttenuation		= 0.006f;
		light.quadraticAttenuation	= 0.007f;
		light.rawCutOffAngle		= 28.0f;
		light.rawOuterCutOffAngle	= 32.0f;
		light.scale					= 0.5f;
		light.minBias				= 0.005f;

		AddLight(light);
	}

	static const char* lightTypes[] = { "Directional", "Point", "Spot" };

	for (int i = 0; i < Scene::LightComponent::LightType::TYPE_COUNT; i++) {
		std::string lightType = "";

		if (i == Scene::LightComponent::LightType::SPOT)
			lightType = "spot";
		if (i == Scene::LightComponent::LightType::DIRECTIONAL)
			lightType = "directional";
		if (i == Scene::LightComponent::LightType::POINT)
			lightType = "point";

		std::string lightCountInfo = lightType + " (" + std::to_string(LightCount[i]) + "/" + std::to_string(LightLimits[i]) + ")";
		ImGui::Text(lightCountInfo.c_str());
	}

	for (int i = 0; i < TotalLights; i++) {
		Scene::LightComponent& light = Lights[i];

		if (!light.IsActive())
			continue;

		std::string light_type = "";

		if (light.type == Scene::LightComponent::LightType::SPOT)
			light_type = "spot";
		if (light.type == Scene::LightComponent::LightType::DIRECTIONAL)
			light_type = "directional";
		if (light.type == Scene::LightComponent::LightType::POINT)
			light_type = "point";

		std::string light_id = "light_" + light_type + "_" + std::to_string(light.index);

		if (ImGui::TreeNode(light_id.c_str())) {
			int selected = light.type;

			if (ImGui::Combo("Light Type", &selected, lightTypes, IM_ARRAYSIZE(lightTypes))) {
				Scene::LightComponent::LightType type = static_cast<Scene::LightComponent::LightType>(selected);

				ChangeLightType(InternalToExternal[light.index], type);
			}

			if (light.type == Scene::LightComponent::LightType::DIRECTIONAL || light.type == Scene::LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Direction X", &light.direction.x, 0.02f, -90.0f, 90.0f, "%.03f");
				ImGui::DragFloat("Direction Y", &light.direction.y, 0.02f, -90.0f, 90.0f, "%.03f");
				ImGui::DragFloat("Direction Z", &light.direction.z, 0.02f, -90.0f, 90.0f, "%.03f");
			} 

			if (light.type == Scene::LightComponent::LightType::POINT || light.type == Scene::LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Position X", &light.position.x, 0.1f, -50.0f, 50.0f, "%0.3f");
				ImGui::DragFloat("Position Y", &light.position.y, 0.1f, -50.0f, 50.0f, "%0.3f");
				ImGui::DragFloat("Position Z", &light.position.z, 0.1f, -50.0f, 50.0f, "%0.3f");
			}

			ImGui::DragFloat("Ambient", &light.ambient, 0.002f, 0.0f, 1.0f, "%0.3f");
			ImGui::DragFloat("Diffuse", &light.diffuse, 0.002f, 0.0f, 1.0f, "%0.3f");
			ImGui::DragFloat("Specular", &light.specular, 0.002f, 0.0f, 1.0f, "%0.3f");

			if (light.type == Scene::LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Cut Off Angle", &light.rawCutOffAngle, 0.002f, 0.0f, 90.0f, "%0.3f");
				ImGui::DragFloat("Outer Cut Off Angle", &light.rawOuterCutOffAngle, 0.002f, 0.0f, 90.0f, "%0.3f");
			}

			if (light.type == Scene::LightComponent::LightType::POINT || light.type == Scene::LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, 0.002f);
				ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, 0.002f);
			}

			ImGui::ColorPicker4("Color", (float*)&light.color);
			ImGui::DragFloat("Scale", &light.scale, 0.002f);

			{	// shadow settings
			
				ImGui::DragFloat("Min Bias", &light.minBias, 0.002f);

				unsigned mask = (1 << 1);
				bool shadowMapEnabled = (light.flags & mask);

				mask = (1 << 2);
				bool pcfFeatureEnabled = (light.flags & mask);

				mask = (1 << 3);
				bool stratifiedPoissionSamplingEnabled = (light.flags & mask);

				mask = (1 << 4);
				bool isLightEnabled = (light.flags & mask);

				if (pcfFeatureEnabled)
					ImGui::DragInt("PCF Samples (sqrt)", &light.pcfSamples, 1, 1, 6);

				if (stratifiedPoissionSamplingEnabled)
					ImGui::DragFloat("Stratified Poisson Sampling Spread", &light.spsSpread, 1.0f, 0.0f, 2000.0f);

				ImGui::Checkbox("Shadow Map Enabled", &shadowMapEnabled);

				if (shadowMapEnabled) {
					ImGui::Checkbox("PCF Enabled", &pcfFeatureEnabled);
					ImGui::Checkbox("Stratified Poisson Sampling Enabled", &stratifiedPoissionSamplingEnabled);
				}

				light.flags = (isLightEnabled << 4) | (stratifiedPoissionSamplingEnabled << 3) | (pcfFeatureEnabled << 2) | (shadowMapEnabled << 1);

				if (ImGui::Button("Render Debug Shadow")) {
					LightShadowRenderDebugIndex = i;
				}
			}

			if (ImGui::Button("Delete Light")) {
				RemoveLight(InternalToExternal[light.index]);
			}

			ImGui::TreePop();
		}
	}
}
