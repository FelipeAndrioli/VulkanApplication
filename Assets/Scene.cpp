#include "Scene.h"

#include "Object.h"
#include "Camera.h"
#include "Mesh.h"
#include "Pipeline.h"

#include "../src/Buffer.h"
#include "../src/BufferHelper.h"
#include "../src/DescriptorSets.h"
#include "../src/LogicalDevice.h"
#include "../src/PhysicalDevice.h"
#include "../src/DescriptorSetLayout.h"
#include "../src/DescriptorPool.h"
#include "../src/GraphicsPipeline.h"

#include "../src/Input/Input.h"
#include "../src/Utils/ModelLoader.h"
#include "../src/Utils/TextureLoader.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		MainCamera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f), 45.0f, m_Width, m_Height);
	}

	Scene::~Scene() {
		for (auto object : RenderableObjects) {
			object->ResetResources();
		}

		RenderableObjects.clear();
		SceneGraphicsPipelines.clear();
		
		delete MainCamera;
	}

	void Scene::AddRenderableObject(Object* object) {
		RenderableObjects.push_back(object);
	}

	void Scene::AddGraphicsPipeline(Assets::GraphicsPipeline newPipeline) {
		SceneGraphicsPipelines.push_back(newPipeline);
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

	void Scene::SetupSceneGeometryBuffer(
		Engine::LogicalDevice& logicalDevice, 
		Engine::PhysicalDevice& physicalDevice, 
		Engine::CommandPool& commandPool) {

		std::vector<Assets::Vertex> vertices;
		std::vector<uint32_t> indices;

		for (auto& renderableObject : RenderableObjects) {
			for (auto& mesh : renderableObject->Meshes) {
				mesh->IndexOffset = indices.size();
				mesh->VertexOffset = vertices.size();

				indices.insert(indices.end(), mesh->Indices.begin(), mesh->Indices.end());
				vertices.insert(vertices.end(), mesh->Vertices.begin(), mesh->Vertices.end());
			}
		}

		VkDeviceSize bufferSize = sizeof(Assets::Vertex) * vertices.size();

		SceneVertexBuffer.reset(new class Engine::Buffer(
			Engine::MAX_FRAMES_IN_FLIGHT,
			logicalDevice,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		));
		SceneVertexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
		Engine::BufferHelper::CopyFromStaging(
			logicalDevice, 
			physicalDevice, 
			commandPool.GetHandle(),
			vertices, 
			SceneVertexBuffer.get());

		bufferSize = sizeof(uint32_t) * indices.size();

		SceneIndexBuffer.reset(new class Engine::Buffer(
			1,
			logicalDevice,
			physicalDevice,
			bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		));
		SceneIndexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Engine::BufferHelper::CopyFromStaging(
			logicalDevice, 
			physicalDevice, 
			commandPool.GetHandle(),
			indices, 
			SceneIndexBuffer.get());
	}
}
