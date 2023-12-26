#pragma once

#include <iostream>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"

namespace Engine {
	class Buffer;
	class DescriptorSets;
	class Material;
}

namespace Assets {
	
	struct Mesh;
	struct Vertex;

	class Camera;

	struct Transform {
		glm::vec3 translation;
		glm::vec3 rotation;
		glm::vec3 scalation;
	};

	class Object {
	public:
		Object();
		~Object();

		virtual void OnCreate() = 0;
		virtual void OnUpdate(float t) = 0;
		virtual void OnUIRender() = 0;

		glm::mat4 GetModelMatrix();

		void ResetResources();
		void SetUniformBufferObject(void* uniformBuffer, size_t uniformBufferSize);
		void SetObjectUniformBuffer(uint32_t currentImage);

		void AddVertices(std::vector<Vertex> vertices);
		void AddVertex(Vertex vertex);
		void AddIndices(std::vector<uint16_t> indices);
		void AddIndex(uint16_t index);
	public:
		std::string ID = "";

		// TODO: Find a way to remove the camera pointer from the object 
		Camera* SceneCamera = nullptr;
		
		Engine::Material* Material = nullptr;
		
		Transform Transformations = {};
	
		std::unique_ptr<class Engine::DescriptorSets> DescriptorSets;
		std::unique_ptr<class Engine::Buffer> UniformBuffer;
		
		size_t UniformBufferObjectSize = 0;
		Mesh* Meshes = nullptr;
	private:
		void* p_UniformBufferObject = nullptr;
	};
}
