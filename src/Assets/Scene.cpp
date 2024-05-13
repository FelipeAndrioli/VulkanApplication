#include "Scene.h"

#include "Object.h"
#include "Camera.h"
#include "Mesh.h"
#include "Pipeline.h"

#include "../Input/Input.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;

		MainCamera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f), 45.0f, m_Width, m_Height);

		/*
		Assets::VertexShader defaultVertexShader = Assets::VertexShader("Default Vertex Shader", "./Assets/Shaders/vert.spv");
		Assets::FragmentShader defaultFragmentShader = Assets::FragmentShader("Default Fragment Shader", "./Assets/Shaders/frag.spv"); 
		Assets::GraphicsPipeline defaultGraphicsPipeline = Assets::GraphicsPipeline("defaultPipeline", defaultVertexShader, defaultFragmentShader);
		AddGraphicsPipeline(defaultGraphicsPipeline);
		*/
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

	void Scene::SetCameraPosition(glm::vec3 pos, float yaw, float pitch) {
		MainCamera->Position = pos;
		MainCamera->Yaw = yaw;
		MainCamera->Pitch = pitch;
		MainCamera->UpdateCameraVectors();
		MainCamera->UpdateProjectionMatrix();
	}

	void Scene::Setup() {
		for (auto& renderableObject : RenderableObjects) {
			for (auto& mesh : renderableObject->Meshes) {
				mesh.IndexOffset = Indices.size();
				mesh.VertexOffset = Vertices.size();

				Indices.insert(Indices.end(), mesh.Indices.begin(), mesh.Indices.end());
				Vertices.insert(Vertices.end(), mesh.Vertices.begin(), mesh.Vertices.end());
			}
		}

		VertexOffset = sizeof(uint32_t) * Indices.size();
	}
}
