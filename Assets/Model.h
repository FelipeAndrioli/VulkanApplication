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

		struct UniformBufferObject {
			glm::mat4 model = glm::mat4(1.0f);
			glm::mat4 view = glm::mat4(1.0f);
			glm::mat4 proj = glm::mat4(1.0f);
		};

		std::unique_ptr<UniformBufferObject> m_UniformBufferObject = std::make_unique<UniformBufferObject>();
		
		virtual void OnCreate() = 0;
		virtual void OnUpdate(float t) = 0;
		virtual void OnUIRender() = 0;

		void SetModelUniformBuffer(uint32_t currentImage);

		UniformBufferObject* GetUniformBufferObject();

		inline uint32_t GetSizeVertices() { return static_cast<uint32_t>(m_Vertices.size()); };
		inline uint32_t GetSizeIndices() { return static_cast<uint32_t>(m_Indices.size()); };

		inline std::vector<Vertex>& GetVertices() { return m_Vertices; };
		inline std::vector<uint16_t>& GetIndices() { return m_Indices; };

		inline void SetVertices(std::vector<Vertex> vertices) { m_Vertices = vertices; };
		inline void SetIndices(std::vector<uint16_t> indices) { m_Indices = indices; };

		void ResetResources();
		glm::mat4 GetModelMatrix();

		Transform m_Transform{};
		std::string m_ID;
		
		std::unique_ptr<class Engine::Buffer> m_UniformBuffer;
		std::unique_ptr<class Engine::DescriptorSets> m_DescriptorSets;
	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;
	};
}
