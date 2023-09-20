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

	void Scene::SetupScene(Engine::LogicalDevice* logicalDevice, Engine::PhysicalDevice* physicalDevice,
		Engine::DescriptorPool* descriptorPool, Engine::DescriptorSetLayout* descriptorSetLayout) {
		m_SceneVertices.clear();
		m_SceneIndices.clear();

		for (auto model : m_SceneModels) {

			{	// uniform buffers
				VkDeviceSize bufferSize = sizeof(Engine::UniformBufferObject);
				model->m_UniformBuffer.reset(new class Engine::Buffer(Engine::MAX_FRAMES_IN_FLIGHT, logicalDevice, physicalDevice,
					bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
				model->m_UniformBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				model->m_UniformBuffer->MapMemory();
			}

			{	// descriptor sets
				model->m_DescriptorSets.reset(new class Engine::DescriptorSets(logicalDevice->GetHandle(), 
					descriptorPool->GetHandle(), descriptorSetLayout->GetHandle(), model->m_UniformBuffer.get()));
			}

			m_SceneVertices.insert(m_SceneVertices.end(), model->GetVertices().begin(), model->GetVertices().end());
			m_SceneIndices.insert(m_SceneIndices.end(), model->GetIndices().begin(), model->GetIndices().end());
		}
	}
}
