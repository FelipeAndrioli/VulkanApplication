#include "Object.h"

namespace Assets {
	Object::Object() {

		Transformations.translation = glm::vec3(0.0f, 0.0f, 0.0f);
		Transformations.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		Transformations.scalation = glm::vec3(1.0f, 1.0f, 1.0f);

		Meshes = new Mesh();
	}

	Object::~Object() {
	
	}

	glm::mat4 Object::GetModelMatrix() {
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::scale(model, Transformations.scalation);
		model = glm::translate(model, Transformations.translation);
		model = glm::rotate(model, glm::radians(Transformations.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Transformations.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Transformations.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		return model;
	}

	void Object::ResetResources() {
		UniformBuffer.reset();
		DescriptorSets.reset();
	}

	void Object::SetUniformBufferObject(void* uniformBuffer, size_t uniformBufferSize) {
		p_UniformBufferObject = uniformBuffer;
		UniformBufferObjectSize = uniformBufferSize;
	}

	void Object::SetObjectUniformBuffer(uint32_t currentImage) {
		if (p_UniformBufferObject == nullptr) return;

		memcpy(UniformBuffer->BufferMemory->MemoryMapped[currentImage], p_UniformBufferObject, UniformBufferObjectSize);
	}

	void Object::AddVertices(std::vector<Vertex> vertices) {
		for (const Vertex& vertex : vertices) {
			AddVertex(vertex);
		}
	}

	void Object::AddVertex(Vertex vertex) {
		Meshes->Vertices.push_back(vertex);
	}

	void Object::AddIndices(std::vector<uint16_t> indices) {
		for (const uint16_t& index : indices) {
			AddIndex(index);
		}
	}

	void Object::AddIndex(uint16_t index) {
		Meshes->Indices.push_back(index);
	}
}