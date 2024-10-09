#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "../Core/Application.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/Settings.h"

#include "../Assets/Camera.h"
#include "../Assets/Model.h"

#include "../Renderer.h"
#include "../LightManager.h"

class ModelViewer : public Application::IScene {
public:
	ModelViewer() {
		settings.Title = "ModelViewer.exe";
		settings.Width = 1600;
		settings.Height = 900;
		settings.uiEnabled = true;
		settings.renderSkybox = false;
	};

	virtual void StartUp() override;
	virtual void CleanUp() override;
	virtual void Update(float d, InputSystem::Input& input) override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) override;
	virtual void RenderUI() override;
	virtual void Resize(uint32_t width, uint32_t height) override;
private:
	Assets::Camera m_Camera = {};

	std::shared_ptr<Assets::Model> m_Dragon;
	std::shared_ptr<Assets::Model> m_Sponza;
	std::shared_ptr<Assets::Model> m_Backpack;
	std::shared_ptr<Assets::Model> m_Window;

	uint32_t m_ScreenWidth = 0;
	uint32_t m_ScreenHeight = 0;
};

void ModelViewer::StartUp() {

	m_ScreenWidth = settings.Width;
	m_ScreenHeight = settings.Height;

	glm::vec3 position = glm::vec3(-5.414f, -0.019f, 5.115f);

	float fov = 45.0f;
	float yaw = -49.6f;
	float pitch = -13.5f;

	m_Camera.Init(position, fov, yaw, pitch, m_ScreenWidth, m_ScreenHeight);

	Renderer::Init();

	LightData sunLight = {};
	sunLight.direction = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
	sunLight.type = LightType::Directional;
	sunLight.ambient = 0.2f;
	sunLight.diffuse = 0.2f;
	sunLight.specular = 0.0f;
	sunLight.scale = 0.2f;
	sunLight.color = glm::vec4(1.0f);

	LightData light = {};
    light.position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	light.type = LightType::PointLight;
	light.linearAttenuation = 0.006f;
	light.quadraticAttenuation = 0.007f;
	light.ambient = 0.1f;
	light.diffuse = 0.5f;
	light.specular = 0.5f;
	light.scale = 0.2f;
	light.color = glm::vec4(1.0f);
	
	LightManager::AddLight(sunLight);
	LightManager::AddLight(light);

	m_Dragon = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf");
	m_Dragon->Name = "Dragon";
	m_Dragon->Transformations.scaleHandler = 11.2f;
	m_Dragon->Transformations.translation = glm::vec3(2.5f, -3.75f, -2.5f);
	m_Dragon->Transformations.rotation.y = -46.9f;

	m_Backpack = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj");
	m_Backpack->Name = "Backpack";
	m_Backpack->Transformations.scaleHandler = 0.3f;
	m_Backpack->Transformations.translation = glm::vec3(0.0f, -3.75f, 0.0f);
	m_Backpack->FlipUvVertically = true;

	m_Sponza = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj");
	//m_Sponza = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/sponza/scene.gltf");
	m_Sponza->Name = "Sponza";
	m_Sponza->Transformations.scaleHandler = 0.008f;
	m_Sponza->Transformations.rotation.y = 45.0f;

	m_Window = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/wooden_window/scene.gltf");
	m_Window->Name = "Window";
	m_Window->Transformations.translation.y = -3.3f;
	m_Window->Transformations.translation.z = -0.9f;
	m_Window->Transformations.rotation.y = -20.0f;
	m_Window->Transformations.scaleHandler = 0.214f;

	Renderer::LoadResources();
}

void ModelViewer::CleanUp() {
	Renderer::Shutdown();
}

void ModelViewer::Update(float d, InputSystem::Input& input) {
	m_Camera.OnUpdate(d, input);
	m_Dragon->OnUpdate(d);
	m_Backpack->OnUpdate(d);
	m_Sponza->OnUpdate(d);
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	Renderer::UpdateGlobalDescriptors(commandBuffer, m_Camera);

	//Renderer::RenderModels(commandBuffer);
	if (settings.renderDefault) {
		Renderer::RenderModel(commandBuffer, *m_Backpack.get());
		Renderer::RenderModel(commandBuffer, *m_Sponza.get());
		Renderer::RenderModel(commandBuffer, *m_Dragon.get());
		Renderer::RenderModelTransparent(commandBuffer, *m_Window.get());
	}

	if (m_Dragon->RenderOutline)
		Renderer::RenderOutline(commandBuffer, *m_Dragon.get());
	if (m_Backpack->RenderOutline)
		Renderer::RenderOutline(commandBuffer, *m_Backpack.get());
	if (m_Sponza->RenderOutline)
		Renderer::RenderOutline(commandBuffer, *m_Sponza.get());
	if (m_Window->RenderOutline)
		Renderer::RenderOutline(commandBuffer, *m_Window.get());

	if (settings.renderWireframe) {
		Renderer::RenderWireframe(commandBuffer, *m_Dragon.get());
		Renderer::RenderWireframe(commandBuffer, *m_Backpack.get());
		Renderer::RenderWireframe(commandBuffer, *m_Sponza.get());
		Renderer::RenderWireframe(commandBuffer, *m_Window.get());
	}

	if (settings.renderLightSources) {
		Renderer::RenderLightSources(commandBuffer);
	}

	if (settings.renderSkybox) {
		Renderer::RenderSkybox(commandBuffer);
	}
}

void ModelViewer::RenderUI() {
	ImGui::SeparatorText("Model Viewer");
	ImGui::Checkbox("Render Default", &settings.renderDefault);
	ImGui::Checkbox("Render Wireframe", &settings.renderWireframe);
	ImGui::Checkbox("Render Skybox", &settings.renderSkybox);
	ImGui::Checkbox("Render Light Sources", &settings.renderLightSources);

	m_Camera.OnUIRender();
	m_Dragon->OnUIRender();
	m_Backpack->OnUIRender();
	m_Sponza->OnUIRender();
	m_Window->OnUIRender();

	Renderer::OnUIRender();
}

void ModelViewer::Resize(uint32_t width, uint32_t height) {
	m_Camera.Resize(width, height);
}

RUN_APPLICATION(ModelViewer)
