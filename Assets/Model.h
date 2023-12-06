#pragma once

#include <iostream>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"

#include "Vertex.h"
#include "Camera.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"
#include "../src/ResourceSet.h"

namespace Assets {

	struct Transform {
		glm::vec3 translation;
		glm::vec3 rotation;
		glm::vec3 scalation;
	};

	class Model {
	public:
		Model();
		~Model();

		virtual void OnCreate() = 0;
		virtual void OnUpdate(float t) = 0;
		virtual void OnUIRender() = 0;

		inline uint32_t GetSizeVertices() { return static_cast<uint32_t>(m_Vertices.size()); };
		inline uint32_t GetSizeIndices() { return static_cast<uint32_t>(m_Indices.size()); };

		inline std::vector<Vertex>& GetVertices() { return m_Vertices; };
		inline std::vector<uint16_t>& GetIndices() { return m_Indices; };

		inline void SetVertices(std::vector<Vertex> vertices) { m_Vertices = vertices; };
		inline void SetIndices(std::vector<uint16_t> indices) { m_Indices = indices; };

		glm::mat4 GetModelMatrix();

		void ResetResources();
		void SetUniformBufferObject(void* uniformBuffer, size_t uniformBufferSize);
		void SetModelUniformBuffer(uint32_t currentImage);

	public:
		std::string ID = "";

		// TODO: Find a way to remove the camera pointer from the model
		Camera* SceneCamera = nullptr;
		
		Engine::ResourceSet* Material = nullptr;
	
		std::unique_ptr<class Engine::DescriptorSets> DescriptorSets;
		std::unique_ptr<class Engine::Buffer> UniformBuffer;
		
		Transform m_Transform{};

		size_t UniformBufferObjectSize = 0;
	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;

		void* p_UniformBufferObject = nullptr;
	};
}
