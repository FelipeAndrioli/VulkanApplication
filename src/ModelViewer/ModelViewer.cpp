#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "../Core/Application.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/Settings.h"
#include "../Core/ResourceManager.h"

#include "../Assets/Camera.h"
#include "../Assets/Model.h"

#include "../Utils/ModelLoader.h"
#include "../Utils/TextureLoader.h"

#include "./PostEffects/PostEffects.h"
#include "./Renderer.h"
#include "./LightManager.h"

class ModelViewer : public Application::IScene {
public:
	ModelViewer() {
		settings.Title		= "ModelViewer.exe";
		settings.Width		= 1600;
		settings.Height		= 900;
		settings.uiEnabled	= true;
	};

	virtual void StartUp	()																		override;
	virtual void CleanUp	()																		override;
	virtual void Update		(const float constantT, const float deltaT, InputSystem::Input& input)	override;
	virtual void RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer)		override;
	virtual void RenderUI	()																		override;
	virtual void Resize		(uint32_t width, uint32_t height)										override;
private:
	Assets::Camera m_Camera			= {};
	Assets::Camera m_SecondCamera	= {};

	std::shared_ptr<Assets::Model> m_Dragon;
	std::shared_ptr<Assets::Model> m_Sponza;
	std::shared_ptr<Assets::Model> m_Backpack;
	std::shared_ptr<Assets::Model> m_Window;
	std::shared_ptr<Assets::Model> m_Window_;

	std::vector<std::shared_ptr<Assets::Model>> m_Models;

	uint32_t m_ScreenWidth			= 0;
	uint32_t m_ScreenHeight			= 0;

	bool m_RenderSkybox				= false;
	bool m_RenderWireframe			= false;
	bool m_RenderLightSources		= false;
	bool m_RenderDepthSwapChain		= false;
	bool m_RenderDepthImGui			= false;
	bool m_RenderNormalsSwapChain	= false;
	bool m_RenderNormalsImGui		= false;
	bool m_RenderNormalMap			= true;

	std::unique_ptr<Graphics::OffscreenRenderTarget>	m_OffscreenRenderTarget;
	std::unique_ptr<Graphics::OffscreenRenderTarget>	m_DebugOffscreenRenderTarget;
	std::unique_ptr<Graphics::OffscreenRenderTarget>	m_DebugOffscreenNormalsRenderTarget;
	std::unique_ptr<Graphics::PostEffectsRenderTarget>	m_PostEffectsRenderTarget;

	VkDescriptorSet m_DebugOffscreenDescriptorSet			= VK_NULL_HANDLE;
	VkDescriptorSet m_DebugOffscreenNormalDescriptorSet		= VK_NULL_HANDLE;
};

void ModelViewer::StartUp() {
	m_ScreenWidth	= settings.Width;
	m_ScreenHeight	= settings.Height;
	
	glm::vec3 position = glm::vec3(-5.414f, -0.019f, 5.115f);

	float fov	= 45.0f;
	float yaw	= -49.6f;
	float pitch = -13.5f;

	m_Camera.Init(position, fov, yaw, pitch, m_ScreenWidth, m_ScreenHeight);

	yaw			= 132.770f;
	pitch		= -10.626f;
	position.x	= 5.966f;
	position.y	= -3.279f;
	position.z	= -5.785f;

	m_SecondCamera.Init(position, fov, yaw, pitch, m_ScreenWidth, m_ScreenHeight);

	Renderer::Init();

	LightData sunLight			= {};
	sunLight.direction			= glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
	sunLight.type				= LightType::Directional;
	sunLight.ambient			= 0.2f;
	sunLight.diffuse			= 0.2f;
	sunLight.specular			= 0.0f;
	sunLight.scale				= 0.2f;
	sunLight.color				= glm::vec4(1.0f);

	LightData light				= {};
    light.position				= glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	light.type					= LightType::PointLight;
	light.linearAttenuation		= 0.006f;
	light.quadraticAttenuation	= 0.007f;
	light.ambient				= 0.1f;
	light.diffuse				= 0.5f;
	light.specular				= 0.5f;
	light.scale					= 0.2f;
	light.color					= glm::vec4(1.0f);
	
	LightManager::AddLight(sunLight);
	LightManager::AddLight(light);

	m_OffscreenRenderTarget					= std::make_unique<Graphics::OffscreenRenderTarget>(m_ScreenWidth, m_ScreenHeight);
	m_DebugOffscreenRenderTarget			= std::make_unique<Graphics::OffscreenRenderTarget>(400, 250);
	m_DebugOffscreenNormalsRenderTarget		= std::make_unique<Graphics::OffscreenRenderTarget>(400, 250);
	m_PostEffectsRenderTarget				= std::make_unique<Graphics::PostEffectsRenderTarget>(m_ScreenWidth, m_ScreenHeight);

	m_DebugOffscreenDescriptorSet			= ImGui_ImplVulkan_AddTexture(
		m_DebugOffscreenRenderTarget->GetColorBuffer().ImageSampler,
		m_DebugOffscreenRenderTarget->GetColorBuffer().ImageView,
		m_DebugOffscreenRenderTarget->GetRenderPass().FinalLayout
	);

	m_DebugOffscreenNormalDescriptorSet		= ImGui_ImplVulkan_AddTexture(
		m_DebugOffscreenNormalsRenderTarget->GetColorBuffer().ImageSampler,
		m_DebugOffscreenNormalsRenderTarget->GetColorBuffer().ImageView,
		m_DebugOffscreenNormalsRenderTarget->GetRenderPass().FinalLayout
	);

	m_Models.emplace_back(Renderer::LoadModel(ModelType::QUAD));
	m_Models[m_Models.size() - 1]->Transformations.rotation.y = 170.0f;

	ResourceManager* rm = ResourceManager::Get();

	int diffuseTextureIndex = rm->AddTexture(TextureLoader::LoadTexture("C:/Users/Felipe/Documents/current_projects/models/textures/brickwall.jpg", Texture::TextureType::DIFFUSE, false, false));
	int normalTextureIndex	= rm->AddTexture(TextureLoader::LoadTexture("C:/Users/Felipe/Documents/current_projects/models/textures/normal_mapping.png", Texture::TextureType::NORMAL, false, false));

	Material material							= {};
	material.Name								= "Normal Map Testing";
	material.MaterialData.DiffuseTextureIndex	= diffuseTextureIndex;
	material.MaterialData.NormalTextureIndex	= normalTextureIndex;

	int materialIndex = rm->AddMaterial(material);

	for (auto& mesh : m_Models[m_Models.size() - 1]->Meshes) {
		mesh.MaterialName	= material.Name;
		mesh.MaterialIndex	= materialIndex;
	}

	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj"));
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler		= 0.3f;
	m_Models[m_Models.size() - 1]->Transformations.translation		= glm::vec3(0.0f, -3.75f, 0.0f);
	m_Models[m_Models.size() - 1]->FlipUvVertically					= true;
	
	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/stanford_dragon_sss_test/scene.gltf"));
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler		= 11.2f;
	m_Models[m_Models.size() - 1]->Transformations.translation		= glm::vec3(2.5f, -3.75f, -2.5f);
	m_Models[m_Models.size() - 1]->Transformations.rotation			= glm::vec3(0.0f, -46.9f, 0.0f);

	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj"));
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler		= 0.008f;
	m_Models[m_Models.size() - 1]->Transformations.rotation.y		= 45.0f;

	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/wooden_window/scene.gltf"));
	m_Models[m_Models.size() - 1]->Transformations.translation		= glm::vec3(0.0f, -3.3f, -0.9f);
	m_Models[m_Models.size() - 1]->Transformations.rotation			= glm::vec3(0.0f, -20.0f, 0.0f);
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler		= 0.214f;
	
	m_Models.emplace_back(Renderer::LoadModel("C:/Users/Felipe/Documents/current_projects/models/actual_models/wooden_window/scene.gltf"));
	m_Models[m_Models.size() - 1]->Transformations.translation		= glm::vec3(2.822f, -3.3f, -3.9f);
	m_Models[m_Models.size() - 1]->Transformations.rotation			= glm::vec3(0.0f, -20.0f, 0.0f);
	m_Models[m_Models.size() - 1]->Transformations.scaleHandler		= 0.214f;
	
	Renderer::LoadResources(*m_OffscreenRenderTarget);
	PostEffects::Initialize();
}

void ModelViewer::CleanUp() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_OffscreenRenderTarget				.reset();
	m_DebugOffscreenRenderTarget		.reset();
	m_DebugOffscreenNormalsRenderTarget	.reset();
	m_PostEffectsRenderTarget			.reset();

	m_Models							.clear();
	Renderer							::Shutdown();
	PostEffects							::Shutdown();
}

void ModelViewer::Update(const float constantT, const float deltaT, InputSystem::Input& input) {
	m_Camera		.OnUpdate(deltaT, input);
	m_SecondCamera	.OnUpdate(deltaT, input);

	for (auto& model : m_Models) {
		model->OnUpdate(deltaT);
	}
}

void ModelViewer::RenderScene(const uint32_t currentFrame, const VkCommandBuffer& commandBuffer) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	Renderer::UpdateGlobalDescriptors(commandBuffer, { m_Camera, m_SecondCamera }, m_RenderNormalMap);

	Renderer::MeshSorter sorter(Renderer::MeshSorter::BatchType::tDefault);
	sorter.SetCamera(m_Camera);

	for (auto& model : m_Models) {
		model->Render(sorter);
	}

	sorter.Sort();

	m_OffscreenRenderTarget->Begin(commandBuffer);

	Renderer::SetCameraIndex(0);
	sorter.RenderMeshes(commandBuffer, Renderer::MeshSorter::DrawPass::tTransparent);
	
	if (m_RenderSkybox) {
		Renderer::RenderSkybox(commandBuffer);
	}

	if (m_RenderLightSources) {
		Renderer::RenderLightSources(commandBuffer);
	}

	if (m_RenderWireframe) {
		for (auto& model : m_Models) {
			Renderer::RenderWireframe(commandBuffer, *model.get());
		}
	}

	for (auto& model : m_Models) {
		if (model->RenderOutline)
			Renderer::RenderOutline(commandBuffer, *model.get());
	}

	m_OffscreenRenderTarget->End(commandBuffer);

	if (m_RenderDepthSwapChain || m_RenderDepthImGui) {
		m_DebugOffscreenRenderTarget->Begin(commandBuffer);

		Renderer::SetCameraIndex(1);
		sorter.ResetDraw();
		sorter.RenderMeshes(commandBuffer, Renderer::MeshSorter::DrawPass::tTransparent, *m_DebugOffscreenRenderTarget.get(), &Renderer::m_RenderDepthPSO);
		m_DebugOffscreenRenderTarget->End(commandBuffer);
	}

	if (m_RenderNormalsSwapChain || m_RenderNormalsImGui) {
		m_DebugOffscreenNormalsRenderTarget->Begin(commandBuffer);
	
		Renderer::SetCameraIndex(0);
		sorter.ResetDraw();
		sorter.RenderMeshes(commandBuffer, Renderer::MeshSorter::DrawPass::tTransparent, *m_DebugOffscreenNormalsRenderTarget.get(), &Renderer::m_RenderNormalsPSO);

		m_DebugOffscreenNormalsRenderTarget->End(commandBuffer);
	}

	m_PostEffectsRenderTarget->Begin(commandBuffer);
	PostEffects::Render(commandBuffer, *m_PostEffectsRenderTarget.get(), m_OffscreenRenderTarget->GetColorBuffer());
	m_PostEffectsRenderTarget->End(commandBuffer);

	// Copy final result from post effects render target to swap chain
	m_PostEffectsRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_PostEffectsRenderTarget->GetColorBuffer());

	if (m_RenderDepthSwapChain) {
		m_DebugOffscreenRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_DebugOffscreenRenderTarget->GetColorBuffer(), ((m_ScreenWidth / 2) + (m_ScreenWidth / 2) / 2) - 50, 100);
	}

	if (m_RenderNormalsSwapChain) {
		m_DebugOffscreenNormalsRenderTarget->ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		gfxDevice->GetSwapChain().RenderTarget->CopyColor(m_DebugOffscreenNormalsRenderTarget->GetColorBuffer(), ((m_ScreenWidth / 2) + (m_ScreenWidth / 2) / 2) - 50, 150 + m_DebugOffscreenRenderTarget->GetExtent().height);
	}
}

void ModelViewer::RenderUI() {
	ImGui::SeparatorText		("Model Viewer");
	ImGui::Checkbox				("Render Wireframe",		&m_RenderWireframe);
	ImGui::Checkbox				("Render Skybox",			&m_RenderSkybox);
	ImGui::Checkbox				("Render Light Sources",	&m_RenderLightSources);
	ImGui::Checkbox				("Render Normal Map",		&m_RenderNormalMap);

	PostEffects::RenderUI();

	ImGui::SeparatorText		("Camera Settings");

	m_Camera.OnUIRender			("Main Camera - Settings");
	m_SecondCamera.OnUIRender	("Secondary Camera - Settings");

	ImGui::SeparatorText		("Models");
	for (auto& model : m_Models) {
		model->OnUIRender();
	}

	Renderer::OnUIRender();
	
	ImGui::SeparatorText	("Debug");
	ImGui::Checkbox			("Render Depth (SwapChain Debug)",		&m_RenderDepthSwapChain);
	ImGui::Checkbox			("Render Normals (SwapChain Debug)",	&m_RenderNormalsSwapChain);

	ImGui::Checkbox			("Render Depth (ImGui Debug)",			&m_RenderDepthImGui);
	ImGui::Checkbox			("Render Normals (ImGui Debug)",		&m_RenderNormalsImGui);

	if (m_RenderDepthImGui) {
		ImGui::Begin("Render Depth");
		ImGui::Image((ImTextureID)m_DebugOffscreenDescriptorSet, ImVec2(m_DebugOffscreenRenderTarget->GetExtent().width, m_DebugOffscreenRenderTarget->GetExtent().height));
		ImGui::End();
	}

	if (m_RenderNormalsImGui) {
		ImGui::Begin("Render Normals");
		ImGui::Image((ImTextureID)m_DebugOffscreenNormalDescriptorSet, ImVec2(m_DebugOffscreenNormalsRenderTarget->GetExtent().width, m_DebugOffscreenNormalsRenderTarget->GetExtent().height));
		ImGui::End();
	}
}

void ModelViewer::Resize(uint32_t width, uint32_t height) {
	m_Camera					.Resize(width, height);
	m_SecondCamera				.Resize(width, height);
	m_OffscreenRenderTarget		->Resize(width, height);
	m_PostEffectsRenderTarget	->Resize(width, height);
}

RUN_APPLICATION(ModelViewer)
