#pragma once

#include <vector>
#include <string>

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

	class Model {
	public:
		Model();
		~Model();

		glm::mat4 GetModelMatrix();

		inline uint32_t GetSizeVertices() { return static_cast<uint32_t>(m_Vertices.size()); };
		inline uint32_t GetSizeIndices() { return static_cast<uint32_t>(m_Indices.size()); };

		inline std::vector<Vertex>& GetVertices() { return m_Vertices; };
		inline std::vector<uint16_t>& GetIndices() { return m_Indices; };

		inline void SetVertices(std::vector<Vertex> vertices) { m_Vertices = vertices; };
		inline void SetIndices(std::vector<uint16_t> indices) { m_Indices = indices; };

		Transform m_Transform{};
		std::string m_ID;
		
		std::unique_ptr<class Engine::Buffer> m_UniformBuffer;
		std::unique_ptr<class Engine::DescriptorSets> m_DescriptorSets;
	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;
	};
}
