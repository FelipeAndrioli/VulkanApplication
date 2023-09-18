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
		//model = glm::rotate(model, m_Transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

		return model;
	}
}