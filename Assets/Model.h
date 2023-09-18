#pragma once

#include <vector>

#include "glm/gtc/matrix_transform.hpp"
#include "Vertex.h"

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

	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;

		Transform m_Transform{};
	};
}
