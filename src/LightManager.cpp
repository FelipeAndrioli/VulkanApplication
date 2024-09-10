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
    sunLight.position = glm::vec4(-9.6f, 18.0f, 5.3f, 1.0f);
	sunLight.type = 1;
	sunLight.ambient = 0.1f;
	sunLight.diffuse = 0.5f;
	sunLight.specular = 0.5f;
	sunLight.scale = 0.2f;
	sunLight.color = glm::vec4(1.0f);

	LightData light2 = {};
    light2.position = glm::vec4(-21.4f, 17.4f, 15.0f, 1.0f);
	light2.type = 1;
	light2.ambient = 0.1f;
	light2.diffuse = 0.5f;
	light2.specular = 0.5f;
	light2.scale = 0.2f;
	light2.color = glm::vec4(1.0f);
	
	AddLight(sunLight);
	AddLight(light2);

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
			ImGui::SliderFloat("Position X", &light.position.x, -50.0f, 50.0f);
			ImGui::SliderFloat("Position Y", &light.position.y, -50.0f, 50.0f);
			ImGui::SliderFloat("Position Z", &light.position.z, -50.0f, 50.0f);

			ImGui::SliderFloat("Ambient", &light.ambient, 0.0f, 1.0f);
			ImGui::SliderFloat("Diffuse", &light.diffuse, 0.0f, 1.0f);
			ImGui::SliderFloat("Specular", &light.specular, 0.0f, 1.0f);

			ImGui::SliderFloat("Light Color R", &light.color.r, 0.0f, 1.0f);
			ImGui::SliderFloat("Light Color G", &light.color.g, 0.0f, 1.0f);
			ImGui::SliderFloat("Light Color B", &light.color.b, 0.0f, 1.0f);
		
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
