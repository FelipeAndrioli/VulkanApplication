#include "Scene.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		//DefaultCamera = Camera();
		DefaultCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f));

		DefaultMaterial = new Engine::Material();
		MapMaterials.emplace(DefaultMaterial->Layout.ID, DefaultMaterial);
	}

	Scene::~Scene() {
		for (auto model : Models) {
			model->ResetResources();
		}

		MapMaterials.erase("Default");
		MapMaterials.clear();

		delete DefaultMaterial;
	}

	void Scene::AddModel(Model* model) {
		Models.push_back(model);
	}

	void Scene::AddMaterial(Engine::Material* material) {
		MapMaterials.emplace(material->Layout.ID, material);
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
				model->Material = MapMaterials.find("Default")->second;
			}
		}

		std::unordered_map<std::string, Engine::Material*>::iterator it;

		for (it = MapMaterials.begin(); it != MapMaterials.end(); it++) {
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
	
	void Scene::SetModelResources(Engine::Material* material, Engine::PhysicalDevice& physicalDevice, Engine::LogicalDevice& logicalDevice) {
		m_HelperVertices.clear();
		m_HelperIndices.clear();

		for (auto model : Models) {
			if (model->Material != material) continue;

			VkDeviceSize bufferSize = sizeof(model->UniformBufferObjectSize);

			model->UniformBuffer.reset(new class Engine::Buffer(Engine::MAX_FRAMES_IN_FLIGHT, logicalDevice, physicalDevice,
				bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
			model->UniformBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			model->UniformBuffer->BufferMemory->MapMemory();

			model->DescriptorSets.reset(new class Engine::DescriptorSets(
				bufferSize,
				logicalDevice.GetHandle(),
				material->GetGraphicsPipeline()->GetDescriptorPool().GetHandle(),
				material->GetGraphicsPipeline()->GetDescriptorSetLayout().GetHandle(),
				model->UniformBuffer.get()
			));

			m_HelperVertices.insert(m_HelperVertices.end(), model->GetVertices().begin(), model->GetVertices().end());
			m_HelperIndices.insert(m_HelperIndices.end(), model->GetIndices().begin(), model->GetIndices().end());
		}
	}
}
