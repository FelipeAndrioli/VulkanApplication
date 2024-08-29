#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "../Application.h"
#include "../Settings.h"
#include "../ConstantBuffers.h"

#include "../Graphics.h"
#include "../GraphicsDevice.h"
#include "../Renderer.h"

#include "../Assets/Camera.h"
#include "../Assets/Model.h"
#include "../Assets/Mesh.h"
#include "../Assets/Utils/MeshGenerator.h"
#include "../Utils/ModelLoader.h"
#include "../Utils/TextureLoader.h"

class ModelViewer : public Engine::Application::IScene {
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
	virtual void Update(float d, Engine::InputSystem::Input& input) override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) override;
	virtual void RenderUI() override;
	virtual void Resize(uint32_t width, uint32_t height) override;
private:
	Assets::Camera* m_Camera = nullptr;

	std::shared_ptr<Model> m_Model;

	uint32_t m_ScreenWidth = 0;
	uint32_t m_ScreenHeight = 0;
};

void ModelViewer::StartUp() {

	m_ScreenWidth = settings.Width;
	m_ScreenHeight = settings.Height;

	m_Camera = new Assets::Camera(glm::vec3(0.6f, 2.1f, 9.2f), 45.0f, -113.0f, -1.7f, m_ScreenWidth, m_ScreenHeight);

	Renderer::Init();

	m_Model = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf");
	m_Model->Name = "Dragon";
	m_Model->Transformations.scaleHandler = 20.0f;
	m_Model->Transformations.translation.x = 0.0f;
	m_Model->Transformations.translation.y = 0.0f;

	/*
	m_Model = Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj");
	m_Model->Name = "Sponza";
	m_Model->Transformations.scaleHandler = 0.008f;
	m_Model->Transformations.translation.x = -10.8f;
	m_Model->Transformations.translation.y = -2.5f;
	m_Model->Transformations.rotation.y = 45.0f;
	m_Model->FlipTexturesVertically = true;
	*/

	Renderer::LoadResources();
}

void ModelViewer::CleanUp() {
	Renderer::Destroy();

	delete m_Camera;
}

void ModelViewer::Update(float d, Engine::InputSystem::Input& input) {
	m_Camera->OnUpdate(d, input);
	m_Model->OnUpdate(d);
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	Renderer::UpdateGlobalDescriptors(commandBuffer, *m_Camera);
	if (settings.renderDefault)
		Renderer::RenderModel(commandBuffer, *m_Model.get());
	if (settings.renderSkybox)
		Renderer::RenderSkybox(commandBuffer);
	if (settings.renderWireframe)
		Renderer::RenderWireframe(commandBuffer, *m_Model.get());
}

void ModelViewer::RenderUI() {
	ImGui::SeparatorText("Model Viewer");
	ImGui::Checkbox("Render Default", &settings.renderDefault);
	ImGui::Checkbox("Render Wireframe", &settings.renderWireframe);
	ImGui::Checkbox("Render Skybox", &settings.renderSkybox);

	m_Camera->OnUIRender();
	m_Model->OnUIRender();
}

void ModelViewer::Resize(uint32_t width, uint32_t height) {
	m_Camera->Resize(width, height);
}

RUN_APPLICATION(ModelViewer)
