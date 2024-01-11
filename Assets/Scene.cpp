#include "Scene.h"

#include "Object.h"
#include "Camera.h"
#include "Mesh.h"
#include "Pipeline.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"
#include "../src/LogicalDevice.h"
#include "../src/PhysicalDevice.h"
#include "../src/DescriptorSetLayout.h"
#include "../src/DescriptorPool.h"
#include "../src/Material.h"
#include "../src/GraphicsPipeline.h"

#include "../src/Input/Input.h"
#include "../src/Utils/ModelLoader.h"
#include "../src/Utils/TextureLoader.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		MainCamera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f), 45.0f, m_Width, m_Height);

		Materials.reset(new class std::map<std::string, std::unique_ptr<Engine::Material>>);
	}

	Scene::~Scene() {
		for (auto object : RenderableObjects) {
			object->ResetResources();
		}

		RenderableObjects.clear();
		Materials.reset();

		GraphicsPipelinesData.clear();

		std::map<std::string, std::unique_ptr<Engine::GraphicsPipeline>>::iterator it;
		for (it = GraphicsPipelines.begin(); it != GraphicsPipelines.end(); it++) {
			it->second.reset();
		}

		GraphicsPipelines.clear();

		delete MainCamera;
	}

	void Scene::AddRenderableObject(Object* object) {
		RenderableObjects.push_back(object);
	}

	void Scene::AddGraphicsPipeline(Assets::GraphicsPipeline newPipeline) {
		GraphicsPipelinesData.push_back(newPipeline);
	}

	void Scene::OnCreate() {
		for (size_t i = 0; i < RenderableObjects.size(); i++) {
			auto object = RenderableObjects[i];

			if (object->ID == "") {
				object->ID = "object_";
				object->ID.append(std::to_string(i));
			}

			object->OnCreate();
		}
	}

	void Scene::OnUIRender() {
		MainCamera->OnUIRender();
		
		for (auto object : RenderableObjects) {
			object->OnUIRender();
		}
	}

	void Scene::OnUpdate(float t, const Engine::InputSystem::Input& input) {

		MainCamera->OnUpdate(t, input);

		for (auto object : RenderableObjects) {
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

		for (const Assets::GraphicsPipeline& graphicsPipeline : GraphicsPipelinesData) {
			GraphicsPipelines.insert(std::make_pair(graphicsPipeline.Name, new class Engine::GraphicsPipeline(
				graphicsPipeline.m_VertexShader,
				graphicsPipeline.m_FragmentShader,
				logicalDevice,
				swapChain,
				depthBuffer,
				renderPass
			)));
		}

		for (Object* object : RenderableObjects) {

			object->SelectedGraphicsPipeline = GraphicsPipelines.find(object->PipelineName)->second.get();

			// temporary
			Engine::Utils::TextureLoader::LoadTexture(
				object->m_Texture,
				object->TexturePath.c_str(),
				logicalDevice,
				physicalDevice,
				commandPool
			);

			VkDeviceSize bufferSize = object->UniformBufferObjectSize;

			object->UniformBuffer.reset(new class Engine::Buffer(Engine::MAX_FRAMES_IN_FLIGHT, logicalDevice, physicalDevice,
				bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
			object->UniformBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			object->UniformBuffer->BufferMemory->MapMemory();

			object->DescriptorSets.reset(new class Engine::DescriptorSets(
				bufferSize,
				logicalDevice.GetHandle(),
				object->SelectedGraphicsPipeline->GetDescriptorPool().GetHandle(),
				object->SelectedGraphicsPipeline->GetDescriptorSetLayout().GetHandle(),
				object->UniformBuffer.get(),
				nullptr,
				false,
				object->m_Texture.get()
			));

			if (object->ModelPath != nullptr) {
				Engine::Utils::ModelLoader::LoadModelAndMaterials(*object, *Materials, logicalDevice, physicalDevice, commandPool);
			}

		}

		std::cout << "Scene Loaded!" << '\n';
	}
}
