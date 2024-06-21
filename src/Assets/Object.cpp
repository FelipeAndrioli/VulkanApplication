#include "Object.h"

#include "Mesh.h"

#include "../UI.h"
#include "../Buffer.h"
#include "../DescriptorSets.h"

namespace Assets {
	Object::Object(glm::vec3 pos) {

		Transformations.scaleHandler = 1.0f;
		Transformations.translation = pos;
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

		for (int i = 0; i < Engine::MAX_FRAMES_IN_FLIGHT; i++) {
			DescriptorSets[i].reset();
		}
	}

	void Object::AddMeshes(std::vector<Assets::Mesh> meshes) {
		Meshes = meshes;

		for (auto mesh : Meshes) {
			IndicesAmount += mesh.Indices.size();
			VerticesAmount += mesh.Vertices.size();
		}
	}

	void Object::AddMesh(Assets::Mesh mesh) {
		mesh.IndexOffset = IndicesAmount;
		mesh.VertexOffset = VerticesAmount;
		Meshes.push_back(mesh);
		IndicesAmount += mesh.Indices.size();
		VerticesAmount += mesh.Vertices.size();
	}

	void Object::OnUIRender() {
		std::string t = "Transformations " + ID;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + ID;
			std::string t_label_y = "Translation y " + ID;
			std::string t_label_z = "Translation z " + ID;

			ImGui::SliderFloat(t_label_x.c_str(), &Transformations.translation.x, -200.0f, 200.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &Transformations.translation.y, -200.0f, 200.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &Transformations.translation.z, -200.0f, 200.0f);

			std::string r_label_x = "Rotation x " + ID;
			std::string r_label_y = "Rotation y " + ID;
			std::string r_label_z = "Rotation z " + ID;
			
			ImGui::SliderFloat(r_label_x.c_str(), &Transformations.rotation.x, -200.0f, 200.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &Transformations.rotation.y, -200.0f, 200.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &Transformations.rotation.z, -200.0f, 200.0f);

			std::string s_label = "Scale Handler " + ID;
			ImGui::SliderFloat(s_label.c_str(), &Transformations.scaleHandler, 0.0f, 2.0f);

			ImGui::Checkbox("Rotate", &m_Rotate);

			ImGui::TreePop();
		}
	}

	void Object::OnUpdate(float t) {
		if (m_Rotate) {
			Transformations.rotation.y += 0.01f * t;

			if (Transformations.rotation.y > 360.0f)
				Transformations.rotation.y = 0.0f;
		}
	}
}
