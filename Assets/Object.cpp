#include "Object.h"

#include "Mesh.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"

namespace Assets {
	Object::Object() {

		Transformations.scaleHandler = 1.0f;
		Transformations.translation = glm::vec3(0.0f, 0.0f, 0.0f);
		Transformations.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	Object::~Object() {

	}

	glm::mat4 Object::GetModelMatrix() {
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::scale(model, glm::vec3(Transformations.scaleHandler, Transformations.scaleHandler, Transformations.scaleHandler));
		model = glm::translate(model, Transformations.translation);
		model = glm::rotate(model, glm::radians(Transformations.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Transformations.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Transformations.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		return model;
	}

	void Object::ResetResources() {
		Meshes.clear();

		DescriptorSets.reset();
	}

	void Object::SetMesh(std::vector<Assets::Mesh>& mesh) {
		Meshes = mesh;
	}
}