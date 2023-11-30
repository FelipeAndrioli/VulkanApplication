#pragma once

#include <iostream>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"
#include "Vertex.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"

namespace Assets {

	struct Transform {
		glm::vec3 translation;
		glm::vec3 rotation;
		glm::vec3 scalation;
	};

	struct UniformBufferObject {
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);
		glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
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

		Transform m_Transform{};
		UniformBufferObject ubo = UniformBufferObject();
		
		std::unique_ptr<class Engine::Buffer> m_UniformBuffer;
		std::unique_ptr<class Engine::DescriptorSets> m_DescriptorSets;

	public:
		std::string ID = "";
		int ResourceSetIndex = -1;
	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;

		void* p_UniformBufferObject = nullptr;
		size_t m_UniformBufferObjectSize = 0;
	};
}
