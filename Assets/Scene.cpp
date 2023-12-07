#include "Scene.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		//DefaultCamera = Camera();
		DefaultCamera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));

		DefaultMaterial = new Engine::Material();
		MapMaterials.emplace(DefaultMaterial->Layout.ID, DefaultMaterial);
	}

	Scene::~Scene() {
		for (auto object : Objects) {
			object->ResetResources();
		}

		MapMaterials.erase("Default");
		MapMaterials.clear();

		delete DefaultMaterial;
	}

	void Scene::AddObject(Object* object) {
		Objects.push_back(object);
	}

	void Scene::AddMaterial(Engine::Material* material) {
		MapMaterials.emplace(material->Layout.ID, material);
	}

	void Scene::OnCreate() {
		for (auto object : Objects) {
			object->OnCreate();
		}
	}

	void Scene::OnUIRender() {
		DefaultCamera.OnUIRender();
		
		for (auto object : Objects) {
			object->OnUIRender();
		}
	}

	void Scene::OnUpdate(float t) {
		DefaultCamera.OnUpdate();

		for (auto object : Objects) {
			object->OnUpdate(t);
		}
	}

	void Scene::SetupScene(Engine::LogicalDevice& logicalDevice, Engine::PhysicalDevice& physicalDevice,
		Engine::CommandPool& commandPool, const Engine::SwapChain& swapChain, const Engine::DepthBuffer& depthBuffer,
		const VkRenderPass& renderPass) {

		for (Object* object : Objects) {
			if (object->Material == nullptr) {
				object->Material = MapMaterials.find("Default")->second;
			}
		}

		std::unordered_map<std::string, Engine::Material*>::iterator it;

		for (it = MapMaterials.begin(); it != MapMaterials.end(); it++) {
			it->second->Init(logicalDevice, physicalDevice, commandPool, swapChain, depthBuffer, renderPass);

			SetObjectResources(it->second, physicalDevice, logicalDevice);

			if (m_HelperVertices.size() > 0) {
				it->second->CreateVertexBuffer(m_HelperVertices, physicalDevice, logicalDevice, commandPool);
			}

			if (m_HelperIndices.size() > 0) {
				it->second->CreateIndexBuffer(m_HelperIndices, physicalDevice, logicalDevice, commandPool);
			}
		}
	}
	
	void Scene::SetObjectResources(Engine::Material* material, Engine::PhysicalDevice& physicalDevice, Engine::LogicalDevice& logicalDevice) {
		m_HelperVertices.clear();
		m_HelperIndices.clear();

		for (auto object : Objects) {
			if (object->Material != material) continue;

			VkDeviceSize bufferSize = sizeof(object->UniformBufferObjectSize);

			object->UniformBuffer.reset(new class Engine::Buffer(Engine::MAX_FRAMES_IN_FLIGHT, logicalDevice, physicalDevice,
				bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
			object->UniformBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			object->UniformBuffer->BufferMemory->MapMemory();

			object->DescriptorSets.reset(new class Engine::DescriptorSets(
				bufferSize,
				logicalDevice.GetHandle(),
				material->GetGraphicsPipeline()->GetDescriptorPool().GetHandle(),
				material->GetGraphicsPipeline()->GetDescriptorSetLayout().GetHandle(),
				object->UniformBuffer.get()
			));

			m_HelperVertices.insert(m_HelperVertices.end(), object->GetVertices().begin(), object->GetVertices().end());
			m_HelperIndices.insert(m_HelperIndices.end(), object->GetIndices().begin(), object->GetIndices().end());
		}
	}
}
