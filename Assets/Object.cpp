#include "Object.h"

#include "Mesh.h"
#include "Camera.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"
#include "../src/Material.h"

namespace Assets {
	Object::Object() {

		Transformations.translation = glm::vec3(0.0f, 0.0f, 0.0f);
		Transformations.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		Transformations.scalation = glm::vec3(1.0f, 1.0f, 1.0f);
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
		for (auto mesh : Meshes) {
			mesh->VertexBuffer.reset();
			mesh->IndexBuffer.reset();
			//mesh->Material.reset();
		}

		//temporary
		m_Texture.reset();

		Meshes.clear();

		GPUDataBuffer.reset();
		DescriptorSets.reset();
	}
}