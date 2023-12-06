#include "Scene.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		//DefaultCamera = Camera();
		DefaultCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f));

		DefaultMaterial = new Engine::ResourceSet();

		MapResourceSets.emplace(DefaultMaterial->MaterialLayout.ID, DefaultMaterial);
	}

	Scene::~Scene() {
		for (auto model : Models) {
			model->ResetResources();
		}

		delete DefaultMaterial;

		MapResourceSets.clear();
	}

	void Scene::AddModel(Model* model) {
		Models.push_back(model);
	}

	void Scene::AddResourceSetLayout(Engine::ResourceSetLayout* resourceSetLayout) {
		m_ResourceSetLayouts.push_back(resourceSetLayout);
	}

	void Scene::AddResourceSet(Engine::ResourceSet* resourceSet) {
		MapResourceSets.emplace(resourceSet->MaterialLayout.ID, resourceSet);
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

		for (Model* model : Models) {
			if (model->Material == nullptr) {
				model->Material = MapResourceSets.find("Default")->second;
			}
		}

		std::unordered_map<std::string, Engine::ResourceSet*>::iterator it;

		for (it = MapResourceSets.begin(); it != MapResourceSets.end(); it++) {
			it->second->Init(logicalDevice, physicalDevice, commandPool, swapChain, depthBuffer, renderPass);

			SetModelResources(it->second, physicalDevice, logicalDevice);

			if (m_HelperVertices.size() > 0) {
				it->second->CreateVertexBuffer(m_HelperVertices, physicalDevice, logicalDevice, commandPool);
			}

			if (m_HelperIndices.size() > 0) {
				it->second->CreateIndexBuffer(m_HelperIndices, physicalDevice, logicalDevice, commandPool);
			}
		}
	}
	
	void Scene::SetModelResources(Engine::ResourceSet* resourceSet, Engine::PhysicalDevice& physicalDevice, Engine::LogicalDevice& logicalDevice) {
		m_HelperVertices.clear();
		m_HelperIndices.clear();

		for (auto model : Models) {
			if (model->Material != resourceSet) continue;

			//VkDeviceSize bufferSize = sizeof(model->UniformBufferObjectSize);
			VkDeviceSize bufferSize = sizeof(Engine::UniformBufferObject);

			model->UniformBuffer.reset(new class Engine::Buffer(Engine::MAX_FRAMES_IN_FLIGHT, logicalDevice, physicalDevice,
				bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
			model->UniformBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			model->UniformBuffer->BufferMemory->MapMemory();

			model->DescriptorSets.reset(new class Engine::DescriptorSets(logicalDevice.GetHandle(), 
				resourceSet->GetGraphicsPipeline()->GetDescriptorPool().GetHandle(), 
				resourceSet->GetGraphicsPipeline()->GetDescriptorSetLayout().GetHandle(), 
				model->UniformBuffer.get())
			);

			m_HelperVertices.insert(m_HelperVertices.end(), model->GetVertices().begin(), model->GetVertices().end());
			m_HelperIndices.insert(m_HelperIndices.end(), model->GetIndices().begin(), model->GetIndices().end());
		}
	}
}
