#include "Model.h"

namespace Assets {
	Model::Model() {

		m_Transform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_Transform.scalation = glm::vec3(1.0f, 1.0f, 1.0f);
	}

	Model::~Model() {
	
	}

	glm::mat4 Model::GetModelMatrix() {
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::scale(model, m_Transform.scalation);
		model = glm::translate(model, m_Transform.translation);
		model = glm::rotate(model, glm::radians(m_Transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(m_Transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(m_Transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		return model;
	}

	void Model::ResetResources() {
		// invoked from scene
		m_UniformBuffer.reset();
		m_DescriptorSets.reset();
	}

	Model::UniformBufferObject* Model::GetUniformBufferObject() {

		m_UniformBufferObject->model = GetModelMatrix();
		m_UniformBufferObject->view = glm::lookAt(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		m_UniformBufferObject->proj = glm::perspective(glm::radians(45.0f), 800.0f / 600, 0.1f, 10.0f);

		// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way
		// to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. If we don't 
		// do this the image will be rendered upside down
		m_UniformBufferObject->proj[1][1] *= -1;

		return m_UniformBufferObject.get();
	}

	void Model::SetModelUniformBuffer(uint32_t currentImage) {
		memcpy(
			m_UniformBuffer->GetBufferMemoryMapped(currentImage),
			GetUniformBufferObject(),
			sizeof(*GetUniformBufferObject())
		);
	}
}