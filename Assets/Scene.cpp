#include "Scene.h"

#include "Object.h"
#include "Camera.h"
#include "Mesh.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"
#include "../src/LogicalDevice.h"
#include "../src/PhysicalDevice.h"
#include "../src/DescriptorSetLayout.h"
#include "../src/DescriptorPool.h"
#include "../src/Material.h"

#include "../src/Input/Input.h"
#include "../src/Utils/ModelLoader.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		MainCamera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f), 45.0f, m_Width, m_Height);

		Materials.reset(new class std::map<std::string, std::unique_ptr<Engine::Material>>);
		DefaultMaterial = new Engine::Material();
		MapMaterials.emplace(DefaultMaterial->Layout.ID, DefaultMaterial);
	}

	Scene::~Scene() {
		for (auto object : Objects) {
			object->ResetResources();
		}

		Objects.clear();
		Materials.reset();

		MapMaterials.erase("Default");
		MapMaterials.clear();

		delete DefaultMaterial;
		delete MainCamera;
	}

	void Scene::AddObject(Object* object) {
		Objects.push_back(object);
	}

	void Scene::AddMaterial(Engine::Material* material) {
		MapMaterials.emplace(material->Layout.ID, material);
	}

	void Scene::OnCreate() {
		for (size_t i = 0; i < Objects.size(); i++) {
			auto object = Objects[i];

			if (object->ID == "") {
				object->ID = "object_";
				object->ID.append(std::to_string(i));
			}

			object->OnCreate();
		}
	}

	void Scene::OnUIRender() {
		MainCamera->OnUIRender();
		
		for (auto object : Objects) {
			object->OnUIRender();
		}
	}

	void Scene::OnUpdate(float t, const Engine::InputSystem::Input& input) {

		MainCamera->OnUpdate(t, input);

		for (auto object : Objects) {
			object->OnUpdate(t);
		}
	}

	void Scene::OnResize(uint32_t width, uint32_t height) {
		if (width == m_Width && height == m_Height) return;

		m_Width = width;
		m_Height = height;

		MainCamera->Resize(m_Width, m_Height);
	}

	void Scene::SetupScene(Engine::LogicalDevice& logicalDevice, Engine::PhysicalDevice& physicalDevice,
		Engine::CommandPool& commandPool, const Engine::SwapChain& swapChain, const Engine::DepthBuffer& depthBuffer,
		const VkRenderPass& renderPass) {

		std::cout << "Starting Scene Loading..." << '\n';

		for (Object* object : Objects) {
			if (object->Material == nullptr) {
				object->Material = MapMaterials.find("Default")->second;
			}
		}

		std::unordered_map<std::string, Engine::Material*>::iterator it;

		for (it = MapMaterials.begin(); it != MapMaterials.end(); it++) {
			it->second->Create(logicalDevice, physicalDevice, commandPool, swapChain, depthBuffer, renderPass);

			SetObjectResources(it->second, physicalDevice, logicalDevice, commandPool);
		}

		std::cout << "Scene Loaded!" << '\n';
	}
	
	void Scene::SetObjectResources(Engine::Material* material, Engine::PhysicalDevice& physicalDevice, 
		Engine::LogicalDevice& logicalDevice, Engine::CommandPool& commandPool) {

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
				object->UniformBuffer.get(),
				nullptr,
				false,
				material->Texture.get()
			));

			if (object->ModelPath != nullptr) {
				Engine::Utils::ModelLoader::LoadModelAndMaterials(*object, *Materials, logicalDevice, physicalDevice, commandPool);
			}
		}
	}
}
