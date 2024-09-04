#include "LightManager.h"

#include <vector>

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/UI.h"

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
    sunLight.position = glm::vec4(0.0f, 6.0f, 0.0f, 1.0f);
	sunLight.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	sunLight.type = 1;
	sunLight.ambientStrength = 0.4f;

	AddLight(sunLight);

	gfxDevice->WriteBuffer(m_LightBuffer, m_Lights.data());

	m_Initialized = true;
}

void LightManager::Shutdown() {
	if (!m_Initialized)
		return;
}

void LightManager::AddLight(LightData lightData) {
	if (m_Lights.size() + 1 == MAX_LIGHTS) {
		std::cout << "Max num of lights reached!" << '\n';
		return;
	}

	m_Lights.push_back(lightData);
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

	ImGui::SeparatorText("Light Manager");
	
	for (int i = 0; i < m_Lights.size(); i++) {
		auto& light = m_Lights[i];

		std::string light_id = "light_";
		light_id += std::to_string(i);
		
		if (ImGui::TreeNode(light_id.c_str())) {
			ImGui::SliderFloat("Position X", &light.position.x, -20.0f, 20.0f);
			ImGui::SliderFloat("Position Y", &light.position.y, -20.0f, 20.0f);
			ImGui::SliderFloat("Position Z", &light.position.z, -20.0f, 20.0f);

			ImGui::SliderFloat("Color R", &light.color.r, 0.0f, 1.0f);
			ImGui::SliderFloat("Color G", &light.color.g, 0.0f, 1.0f);
			ImGui::SliderFloat("Color B", &light.color.b, 0.0f, 1.0f);
			
			ImGui::SliderFloat("Light Intensity", &light.color.a, 0.0f, 1.0f);

			ImGui::SliderFloat("Ambient Strength R", &light.ambientStrength, 0.0f, 1.0f);

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
