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

class ModelViewer : public Application::IScene {
public:
	ModelViewer() {
		settings.Title = "ModelViewer.exe";
		settings.Width = 1600;
		settings.Height = 900;
		settings.uiEnabled = true;
		settings.renderSkybox = true;
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

	uint32_t m_ScreenWidth = 0;
	uint32_t m_ScreenHeight = 0;
};

void ModelViewer::StartUp() {

	m_ScreenWidth = settings.Width;
	m_ScreenHeight = settings.Height;

	glm::vec3 position = glm::vec3(0.6f, 2.1f, 9.2f);

	float fov = 45.0f;
	float yaw = -113.0f;
	float pitch = -1.7f;

	m_Camera.Init(position, fov, yaw, pitch, m_ScreenWidth, m_ScreenHeight);

	Renderer::Init();

	m_Dragon = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf");
	m_Dragon->Name = "Dragon";
	m_Dragon->Transformations.scaleHandler = 20.0f;
	m_Dragon->Transformations.translation.x = 0.0f;
	m_Dragon->Transformations.translation.y = 0.0f;

	m_Backpack = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj");
	m_Backpack->Name = "Backpack";
	m_Backpack->Transformations.scaleHandler = 1.0f;
	m_Backpack->Transformations.translation.x = -4.44f;
	m_Backpack->Transformations.translation.y = 1.4f;
	m_Backpack->FlipUvVertically = true;

	/*
	m_Sponza = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj");
	m_Sponza->Name = "Sponza";
	m_Sponza->Transformations.scaleHandler = 0.008f;
	m_Sponza->Transformations.translation.x = -10.8f;
	m_Sponza->Transformations.translation.y = -2.5f;
	m_Sponza->Transformations.rotation.y = 45.0f;
	*/

	Renderer::LoadResources();
}

void ModelViewer::CleanUp() {
	Renderer::Shutdown();
}

void ModelViewer::Update(float d, InputSystem::Input& input) {
	m_Camera.OnUpdate(d, input);
	m_Dragon->OnUpdate(d);
	m_Backpack->OnUpdate(d);
	//m_Sponza->OnUpdate(d);
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	Renderer::UpdateGlobalDescriptors(commandBuffer, m_Camera);

	if (settings.renderDefault) {
		Renderer::RenderModel(commandBuffer, *m_Dragon.get());
		Renderer::RenderModel(commandBuffer, *m_Backpack.get());
		//Renderer::RenderModel(commandBuffer, *m_Sponza.get());
	}

	if (settings.renderWireframe) {
		Renderer::RenderWireframe(commandBuffer, *m_Dragon.get());
		Renderer::RenderWireframe(commandBuffer, *m_Backpack.get());
		//Renderer::RenderWireframe(commandBuffer, *m_Sponza.get());
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
	//m_Sponza->OnUIRender();

	Renderer::OnUIRender();
}

void ModelViewer::Resize(uint32_t width, uint32_t height) {
	m_Camera.Resize(width, height);
}

RUN_APPLICATION(ModelViewer)
