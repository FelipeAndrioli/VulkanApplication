#include "Scene.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

	}

	void Scene::AddModel(Model* model) {
		m_SceneModels.push_back(model);
	}

	void Scene::DeleteModels() {
		for (auto model : m_SceneModels) {
			delete model;
		}
	}

	void Scene::SetupScene() {
		m_SceneVertices.clear();
		m_SceneIndices.clear();

		for (auto model : m_SceneModels) {
			m_SceneVertices.insert(m_SceneVertices.end(), model->GetVertices().begin(), model->GetVertices().end());
			m_SceneIndices.insert(m_SceneIndices.end(), model->GetIndices().begin(), model->GetIndices().end());
		}
	}
}
