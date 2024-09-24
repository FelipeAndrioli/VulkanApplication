#include "LightManager.h"

#include "../Core/UI.h"
#include "../Core/ConstantBuffers.h"

#include "./Renderer.h"

namespace LightManager {
	bool m_Initialized = false;

	std::vector<LightData> m_Lights;

	Graphics::Buffer m_LightBuffer;	
}

void LightManager::Init() {
	if (m_Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_LightBuffer = gfxDevice->CreateBuffer(sizeof(LightData) * MAX_LIGHTS);

	m_Initialized = true;
}

void LightManager::Shutdown() {
	if (!m_Initialized)
		return;

	m_Lights.clear();
}

void LightManager::AddLight(LightData& light) {
	if (m_Lights.size() + 1 == MAX_LIGHTS) {
		std::cout << "Max num of lights reached!" << '\n';
		return;
	}

	m_Lights.push_back(light);
}

void LightManager::UpdateBuffer() {
	if (!m_Initialized)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

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
	}

	gfxDevice->UpdateBuffer(m_LightBuffer, m_Lights.data());
}

void LightManager::OnUIRender() {
	if (!m_Initialized)
		return;

	ImGui::SeparatorText("Light Manager");
	
	for (int i = 0; i < m_Lights.size(); i++) {
		auto& light = m_Lights[i];

		std::string light_id = "light_";
		light_id += std::to_string(i);

		if (ImGui::TreeNode(light_id.c_str())) {
			static const char* lightTypes[] = { "Directional", "Point", "Spot" };
			int selected = light.type;

			if (ImGui::Combo("Light Type", &selected, lightTypes, IM_ARRAYSIZE(lightTypes))) {
				light.type = static_cast<LightType>(selected);
			}

			if (light.type == LightType::Directional || light.type == LightType::SpotLight) {
				ImGui::DragFloat("Direction X", &light.direction.x, 0.002f, -1.0f, 1.0f, "%.03f");
				ImGui::DragFloat("Direction Y", &light.direction.y, 0.002f, -1.0f, 1.0f, "%.03f");
				ImGui::DragFloat("Direction Z", &light.direction.z, 0.002f, -1.0f, 1.0f, "%.03f");
			} 

			if (light.type == LightType::PointLight || light.type == LightType::SpotLight) {
				ImGui::DragFloat("Position X", &light.position.x, 0.1f, -50.0f, 50.0f, "%0.3f");
				ImGui::DragFloat("Position Y", &light.position.y, 0.1f, -50.0f, 50.0f, "%0.3f");
				ImGui::DragFloat("Position Z", &light.position.z, 0.1f, -50.0f, 50.0f, "%0.3f");
			}

			ImGui::DragFloat("Ambient", &light.ambient, 0.002f, 0.0f, 1.0f, "%0.3f");
			ImGui::DragFloat("Diffuse", &light.diffuse, 0.002f, 0.0f, 1.0f, "%0.3f");
			ImGui::DragFloat("Specular", &light.specular, 0.002f, 0.0f, 1.0f, "%0.3f");

			if (light.type == LightType::SpotLight) {
				ImGui::DragFloat("Cut Off Angle", &light.rawCutOffAngle, 0.002f, 0.0f, 90.0f, "%0.3f");
				ImGui::DragFloat("Outer Cut Off Angle", &light.rawOuterCutOffAngle, 0.002f, 0.0f, 90.0f, "%0.3f");
			}

			if (light.type == LightType::PointLight || light.type == LightType::SpotLight) {
				ImGui::DragFloat("Linear Attenuation", &light.linearAttenuation, 0.002f);
				ImGui::DragFloat("Quadratic Attenuation", &light.quadraticAttenuation, 0.002f);
			}

			ImGui::ColorPicker4("Color", (float*)&light.color);
			ImGui::DragFloat("Scale", &light.scale, 0.002f);
			ImGui::TreePop();
		}
	}
}

int LightManager::GetTotalLights() {
	return m_Lights.size();
}

Graphics::Buffer& LightManager::GetLightBuffer() {
	return m_LightBuffer;
}

std::vector<LightData>& LightManager::GetLights() {
	return m_Lights;
}
