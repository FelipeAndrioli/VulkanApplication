#include "Scene.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;
	}

	Scene::~Scene() {
		for (auto model : p_Models) {
			model->ResetResources();
		}

		for (auto resourceSet : m_ResourceSets) {
			delete resourceSet;
		}
	}

	void Scene::AddModel(Model* model) {
		p_Models.push_back(model);
	}

	void Scene::OnCreate() {
		for (auto model : p_Models) {
			model->OnCreate();
		}
	}

	void Scene::OnUIRender() {
		for (auto model : p_Models) {
			model->OnUIRender();
		}
	}

	void Scene::OnUpdate(float t) {
		for (auto model : p_Models) {
			model->OnUpdate(t);
		}
	}

	void Scene::SetupScene(Engine::RenderLayout* renderLayout, Engine::LogicalDevice* logicalDevice, 
		Engine::PhysicalDevice* physicalDevice, Engine::CommandPool* commandPool, 
		Engine::SwapChain* swapChain) {
		
		m_ResourceSets.resize(renderLayout->GetGraphicsPipelineLayouts().size());

		for (size_t i = 0; i < renderLayout->GetGraphicsPipelineLayouts().size(); i++) {
			m_ResourceSets[i] = new Engine::ResourceSet(renderLayout->GetGraphicsPipelineLayout(i), 
				logicalDevice, physicalDevice, commandPool, swapChain, p_Models);
		}
	}

	void Scene::Resize() {
		for (size_t i = 0; i < m_ResourceSets.size(); i++) {
			m_ResourceSets[i]->Resize();
		}
	}
}
