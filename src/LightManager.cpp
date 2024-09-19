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

	LightData sunLight = {};
	sunLight.direction = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
	sunLight.type = LightType::Directional;
	sunLight.ambient = 0.2f;
	sunLight.diffuse = 0.2f;
	sunLight.specular = 0.0f;
	sunLight.scale = 0.2f;
	sunLight.color = glm::vec4(1.0f);

	LightData light = {};
    //light.position = glm::vec4(-21.4f, 17.4f, 15.0f, 0.0f);
    light.position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	light.type = LightType::PointLight;
	light.linearAttenuation = 0.006f;
	light.quadraticAttenuation = 0.007f;
	light.ambient = 0.1f;
	light.diffuse = 0.5f;
	light.specular = 0.5f;
	light.scale = 0.2f;
	light.color = glm::vec4(1.0f);
	
	AddLight(sunLight);
	AddLight(light);

	gfxDevice->WriteBuffer(m_LightBuffer, m_Lights.data());

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
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::scale(model, glm::vec3(light.scale, light.scale, light.scale));
		model = glm::translate(model, glm::vec3(light.position));

		light.model = model;
		light.cutOffAngle = glm::cos(glm::radians(light.rawCutOffAngle));
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
				ImGui::SliderFloat("Direction X", &light.direction.x, -1.0f, 1.0f);
				ImGui::SliderFloat("Direction Y", &light.direction.y, -1.0f, 1.0f);
				ImGui::SliderFloat("Direction Z", &light.direction.z, -1.0f, 1.0f);
			} 

			if (light.type == LightType::PointLight || light.type == LightType::SpotLight) {
				ImGui::SliderFloat("Position X", &light.position.x, -50.0f, 50.0f);
				ImGui::SliderFloat("Position Y", &light.position.y, -50.0f, 50.0f);
				ImGui::SliderFloat("Position Z", &light.position.z, -50.0f, 50.0f);
			}

			ImGui::SliderFloat("Ambient", &light.ambient, 0.0f, 1.0f);
			ImGui::SliderFloat("Diffuse", &light.diffuse, 0.0f, 1.0f);
			ImGui::SliderFloat("Specular", &light.specular, 0.0f, 1.0f);

			if (light.type == LightType::SpotLight) {
				ImGui::SliderFloat("Cut Off Angle", &light.rawCutOffAngle, 0.0f, 90.0f);
			}

			if (light.type == LightType::PointLight || light.type == LightType::SpotLight) {
				ImGui::SliderFloat("Linear Attenuation", &light.linearAttenuation, 0.0f, 2.0f);
				ImGui::SliderFloat("Quadratic Attenuation", &light.quadraticAttenuation, 0.0f, 2.0f);
			}

			ImGui::ColorPicker4("Color", (float*)&light.color);
			ImGui::SliderFloat("Scale", &light.scale, 0.0f, 10.0f);
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
