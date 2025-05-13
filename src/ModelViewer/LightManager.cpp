#include "LightManager.h"

#include "../Core/UI.h"
#include "../Core/ConstantBuffers.h"

#include "../Assets/ShadowCamera.h"

#include "./Renderer.h"

namespace LightManager {
	bool m_Initialized = false;
	
	int m_LightShadowRenderDebugIndex = -1;

	std::vector<Scene::LightComponent> m_Lights;

	Graphics::Buffer m_LightBuffer;	
}

void LightManager::Init() {
	if (m_Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_LightBuffer = gfxDevice->CreateBuffer(sizeof(Scene::LightComponent) * MAX_LIGHT_SOURCES);

	m_Initialized = true;
}

void LightManager::Shutdown() {
	if (!m_Initialized)
		return;

	m_Lights.clear();
}

void LightManager::AddLight(Scene::LightComponent& light) {
	if (m_Lights.size() + 1 > MAX_LIGHT_SOURCES) {
		std::cout << "Max num of lights reached!" << '\n';
		return;
	}

	light.index = m_Lights.size();

	m_Lights.push_back(light);
}

void LightManager::DeleteLight(Scene::LightComponent& light) {
	if (!m_Initialized)
		return;

	DeleteLight(light.index);
}

void LightManager::DeleteLight(int lightIndex) {
	if (!m_Initialized)
		return;

	if (m_LightShadowRenderDebugIndex == lightIndex)
		m_LightShadowRenderDebugIndex = -1;

	m_Lights.erase(m_Lights.begin() + lightIndex);

	for (int i = 0; i < m_Lights.size(); i++) {
		if (m_LightShadowRenderDebugIndex == m_Lights[i].index)
			m_LightShadowRenderDebugIndex = i;

		m_Lights[i].index = i;
	}
}

void LightManager::Update(Assets::ShadowCamera& camera) {

	for (auto& light : m_Lights) {
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
}

void LightManager::UpdateBuffer() {
	if (!m_Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->UpdateBuffer(m_LightBuffer, m_Lights.data());
}

void LightManager::OnUIRender() {
	if (!m_Initialized)
		return;

	using namespace Scene;

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

		AddLight(light);
	}

	for (int i = 0; i < m_Lights.size(); i++) {
		auto& light = m_Lights[i];

		std::string light_id = "light_";
		light_id += std::to_string(i);

		if (ImGui::TreeNode(light_id.c_str())) {
			static const char* lightTypes[] = { "Directional", "Point", "Spot" };
			int selected = light.type;

			if (ImGui::Combo("Light Type", &selected, lightTypes, IM_ARRAYSIZE(lightTypes))) {
				light.type = static_cast<LightComponent::LightType>(selected);
			}

			if (light.type == LightComponent::LightType::DIRECTIONAL || light.type == LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Direction X", &light.direction.x, 0.02f, -90.0f, 90.0f, "%.03f");
				ImGui::DragFloat("Direction Y", &light.direction.y, 0.02f, -90.0f, 90.0f, "%.03f");
				ImGui::DragFloat("Direction Z", &light.direction.z, 0.02f, -90.0f, 90.0f, "%.03f");
			} 

			if (light.type == LightComponent::LightType::POINT || light.type == LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Position X", &light.position.x, 0.1f, -50.0f, 50.0f, "%0.3f");
				ImGui::DragFloat("Position Y", &light.position.y, 0.1f, -50.0f, 50.0f, "%0.3f");
				ImGui::DragFloat("Position Z", &light.position.z, 0.1f, -50.0f, 50.0f, "%0.3f");
			}

			ImGui::DragFloat("Ambient", &light.ambient, 0.002f, 0.0f, 1.0f, "%0.3f");
			ImGui::DragFloat("Diffuse", &light.diffuse, 0.002f, 0.0f, 1.0f, "%0.3f");
			ImGui::DragFloat("Specular", &light.specular, 0.002f, 0.0f, 1.0f, "%0.3f");

			if (light.type == LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Cut Off Angle", &light.rawCutOffAngle, 0.002f, 0.0f, 90.0f, "%0.3f");
				ImGui::DragFloat("Outer Cut Off Angle", &light.rawOuterCutOffAngle, 0.002f, 0.0f, 90.0f, "%0.3f");
			}

			if (light.type == LightComponent::LightType::POINT || light.type == LightComponent::LightType::SPOT) {
				ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, 0.002f);
				ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, 0.002f);
			}

			ImGui::ColorPicker4("Color", (float*)&light.color);
			ImGui::DragFloat("Scale", &light.scale, 0.002f);

			if (ImGui::Button("Render Debug Shadow")) {
				m_LightShadowRenderDebugIndex = i;
			}

			if (ImGui::Button("Delete Light")) {
				DeleteLight(i);
			}

			ImGui::TreePop();
		}
	}
}

int LightManager::GetTotalLights() {
	return m_Lights.size();
}

int LightManager::GetLightShadowDebugIndex() {
	return m_LightShadowRenderDebugIndex;
}

Graphics::Buffer& LightManager::GetLightBuffer() {
	return m_LightBuffer;
}

std::vector<Scene::LightComponent>& LightManager::GetLights() {
	return m_Lights;
}
