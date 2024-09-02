#include "LightManager.h"

#include <vector>

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"

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
	sunLight.Position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	sunLight.Color= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	sunLight.Type = 1;

	LightData sunLight1 = {};
	sunLight1.Position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	sunLight1.Color= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	sunLight1.Type = 1;

	LightData sunLight2 = {};
	sunLight2.Position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	sunLight2.Color= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	sunLight2.Type = 1;

	LightData sunLight3 = {};
	sunLight3.Position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	sunLight3.Color= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	sunLight3.Type = 1;

	LightData sunLight4 = {};
	sunLight4.Position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	sunLight4.Color= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	sunLight4.Type = 1;

	AddLight(sunLight);
	AddLight(sunLight1);
	AddLight(sunLight2);
	AddLight(sunLight3);
	AddLight(sunLight4);

	std::cout << m_Lights.size() << " loaded lights!" << '\n';

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

Graphics::Buffer& LightManager::GetLightBuffer() {
	return m_LightBuffer;
}
