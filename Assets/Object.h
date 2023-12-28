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
	public:
		std::string ID = "";
		const char* ModelPath = nullptr;
		std::vector<Mesh> Meshes;
		Engine::Material* Material = nullptr;

		// TODO: Remove camera from here 
		Camera* SceneCamera = nullptr;
		
		Transform Transformations = {};
	
		std::unique_ptr<class Engine::DescriptorSets> DescriptorSets;
		std::unique_ptr<class Engine::Buffer> UniformBuffer;
		
		size_t UniformBufferObjectSize = 0;
	private:
		void* p_UniformBufferObject = nullptr;
	};
}
