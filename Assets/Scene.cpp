#include "Scene.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		//DefaultCamera = Camera();
		DefaultCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f));
	}

	Scene::~Scene() {
		for (auto model : Models) {
			model->ResetResources();
		}

		for (Engine::ResourceSet* resourceSet : ResourceSets) {
			delete resourceSet;
		}
	}

	void Scene::AddModel(Model* model) {
		Models.push_back(model);

		for (size_t i = 0; i < Models.size(); i++) {
			if (Models.size() == 1) break;

			for (size_t j = i + 1; j < Models.size(); j++) {
				if (Models[i]->ResourceSetIndex < Models[j]->ResourceSetIndex) {
					Model* tempModel = Models[i];
					Models[i] = Models[j];
					Models[j] = tempModel;
				}
			}
		}
	}

	void Scene::AddResourceSetLayout(Engine::ResourceSetLayout* resourceSetLayout) {
		m_ResourceSetLayouts.push_back(resourceSetLayout);
	}

	void Scene::OnCreate() {
		for (auto model : Models) {
			model->OnCreate();
		}
	}

	void Scene::OnUIRender() {
		DefaultCamera.OnUIRender();
		
		for (auto model : Models) {
			model->OnUIRender();
		}
	}

	void Scene::OnUpdate(float t) {
		DefaultCamera.OnUpdate();

		for (auto model : Models) {
			model->OnUpdate(t);
		}
	}

	void Scene::SetupScene(Engine::LogicalDevice& logicalDevice, Engine::PhysicalDevice& physicalDevice, 
		Engine::CommandPool& commandPool, const Engine::SwapChain& swapChain, const Engine::DepthBuffer& depthBuffer,
		const VkRenderPass& renderPass) {
	
		int maxIndex = 0;

		// Add default
		m_ResourceSetLayouts.push_back(new Engine::ResourceSetLayout{});

		for (Engine::ResourceSetLayout* resourceSetLayout : m_ResourceSetLayouts) {
			if (resourceSetLayout->ResourceSetIndex == -1) {
				resourceSetLayout->ResourceSetIndex = (int)m_ResourceSetLayouts.size() - 1;
			}
		
			if (resourceSetLayout->ResourceSetIndex > maxIndex) maxIndex = resourceSetLayout->ResourceSetIndex;
		}

		ResourceSets.resize(maxIndex + 1);

		for (Model* model : Models) {
			if (model->ResourceSetIndex == -1) model->ResourceSetIndex = (int)m_ResourceSetLayouts.size() - 1;
		}

		for (Engine::ResourceSetLayout* resourceSetLayout : m_ResourceSetLayouts) {
			ResourceSets[resourceSetLayout->ResourceSetIndex] = new Engine::ResourceSet(*resourceSetLayout, logicalDevice,
				physicalDevice, commandPool, swapChain, depthBuffer, renderPass, Models);
		}
	}
}
