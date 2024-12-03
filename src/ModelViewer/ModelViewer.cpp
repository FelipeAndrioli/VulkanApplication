#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "../Core/Application.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/Settings.h"
#include "../Core/RenderPassManager.h"

#include "../Assets/Camera.h"
#include "../Assets/Model.h"

#include "../Renderer.h"
#include "../LightManager.h"

#include "./GrayScale.h"

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
	std::shared_ptr<Assets::Model> m_Window_;

	std::vector<std::shared_ptr<Assets::Model>> m_Models;

	uint32_t m_ScreenWidth = 0;
	uint32_t m_ScreenHeight = 0;

	Graphics::RenderPass m_ColorRenderPass = {};
	Graphics::RenderPass m_PostRenderPass = {};
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

	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf"));
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler = 11.2f;
	m_Models[m_Models.size() - 1]->Transformations.translation = glm::vec3(2.5f, -3.75f, -2.5f);
	m_Models[m_Models.size() - 1]->Transformations.rotation = glm::vec3(0.0f, -46.9f, 0.0f);

	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj"));
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler = 0.3f;
	m_Models[m_Models.size() - 1]->Transformations.translation = glm::vec3(0.0f, -3.75f, 0.0f);
	m_Models[m_Models.size() - 1]->FlipUvVertically = true;

	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj"));
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler = 0.008f;
	m_Models[m_Models.size() - 1]->Transformations.rotation.y = 45.0f;

	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/wooden_window/scene.gltf"));
	m_Models[m_Models.size() - 1]->Transformations.translation = glm::vec3(0.0f, -3.3f, -0.9f);
	m_Models[m_Models.size() - 1]->Transformations.rotation = glm::vec3(0.0f, -20.0f, 0.0f);
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler = 0.214f;
	
	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/wooden_window/scene.gltf"));
	m_Models[m_Models.size() - 1]->Transformations.translation = glm::vec3(2.822f, -3.3f, -3.9f);
	m_Models[m_Models.size() - 1]->Transformations.rotation = glm::vec3(0.0f, -20.0f, 0.0f);
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler = 0.214f;

	Renderer::LoadResources();

	GrayScale::Initialize();
}

void ModelViewer::CleanUp() {
	m_Models.clear();
	GrayScale::Shutdown();
	Renderer::Shutdown();
}

void ModelViewer::Update(float d, InputSystem::Input& input) {
	m_Camera.OnUpdate(d, input);

	for (auto& model : m_Models) {
		model->OnUpdate(d);
	}
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->BeginRenderPass(Graphics::g_ColorRenderPass, commandBuffer);

	Renderer::UpdateGlobalDescriptors(commandBuffer, m_Camera);

	Renderer::MeshSorter sorter(Renderer::MeshSorter::BatchType::tDefault);
	sorter.SetCamera(m_Camera);

	for (auto& model : m_Models) {
		model->Render(sorter);
	}

	sorter.Sort();
	sorter.RenderMeshes(commandBuffer, Renderer::MeshSorter::DrawPass::tTransparent);
	
	if (settings.renderSkybox) {
		Renderer::RenderSkybox(commandBuffer);
	}

	gfxDevice->EndRenderPass(commandBuffer);

	gfxDevice->BeginRenderPass(Graphics::g_PostEffectsRenderPass, commandBuffer);
	GrayScale::Render(commandBuffer, Graphics::g_PostEffectsRenderPass);
	
	gfxDevice->EndRenderPass(commandBuffer);

	/*
	for (auto& model : m_Models) {
		if (model->RenderOutline)
			Renderer::RenderOutline(commandBuffer, *model.get());
	}

	if (settings.renderWireframe) {
		for (auto& model : m_Models) {
			Renderer::RenderWireframe(commandBuffer, *model.get());
		}
	}

	if (settings.renderLightSources) {
		Renderer::RenderLightSources(commandBuffer);
	}

	*/
}

void ModelViewer::RenderUI() {
	ImGui::SeparatorText("Model Viewer");
	ImGui::Checkbox("Render Default", &settings.renderDefault);
	ImGui::Checkbox("Render Wireframe", &settings.renderWireframe);
	ImGui::Checkbox("Render Skybox", &settings.renderSkybox);
	ImGui::Checkbox("Render Light Sources", &settings.renderLightSources);

	m_Camera.OnUIRender();


	for (auto& model : m_Models) {
		model->OnUIRender();
	}

	Renderer::OnUIRender();
}

void ModelViewer::Resize(uint32_t width, uint32_t height) {
	m_Camera.Resize(width, height);
}

RUN_APPLICATION(ModelViewer)
