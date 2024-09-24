#include "Model.h"

#include <iostream>

#include "../Core/GraphicsDevice.h"
#include "../Core/UI.h"

namespace Assets {
	Model::~Model() {
		std::cout << "Destroying model " << Name << '\n';
		Destroy();
	}

	void Model::Destroy() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		gfxDevice->DestroyDescriptorSetLayout(ModelDescriptorSetLayout);

		Meshes.clear();
		gfxDevice->DestroyBuffer(DataBuffer);
	}

	glm::mat4 Model::GetModelMatrix() {
		glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -PivotVector);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(Transformations.scaleHandler));

		glm::mat4 rotation = glm::mat4(1.0f);
		rotation = glm::rotate(rotation, glm::radians(Transformations.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		rotation = glm::rotate(rotation, glm::radians(Transformations.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotation = glm::rotate(rotation, glm::radians(Transformations.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 toPosition = glm::translate(glm::mat4(1.0f), Transformations.translation);

		glm::mat4 model = toPosition * rotation * scale * toOrigin;

		return model;
	}


	void Model::OnUIRender() {
		std::string t = "Transformations " + Name;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + Name;
			std::string t_label_y = "Translation y " + Name;
			std::string t_label_z = "Translation z " + Name;

			ImGui::DragFloat(t_label_x.c_str(), &Transformations.translation.x, 0.002f);
			ImGui::DragFloat(t_label_y.c_str(), &Transformations.translation.y, 0.002f);
			ImGui::DragFloat(t_label_z.c_str(), &Transformations.translation.z, 0.002f);

			std::string r_label_x = "Rotation x " + Name;
			std::string r_label_y = "Rotation y " + Name;
			std::string r_label_z = "Rotation z " + Name;

			ImGui::DragFloat(r_label_x.c_str(), &Transformations.rotation.x, 0.002f);
			ImGui::DragFloat(r_label_y.c_str(), &Transformations.rotation.y, 0.002f);
			ImGui::DragFloat(r_label_z.c_str(), &Transformations.rotation.z, 0.002f);

			std::string s_label = "Scale Handler " + Name;
			ImGui::DragFloat(s_label.c_str(), &Transformations.scaleHandler, 0.002f);

			ImGui::Checkbox("Rotate", &Rotate);
			ImGui::Checkbox("Flip UV Vertically", &FlipUvVertically);

			ImGui::TreePop();
		}
	}

	void Model::OnUpdate(float t) {
		if (Rotate) {
			Transformations.rotation.y += 0.01f * t;

			if (Transformations.rotation.y > 360.0f)
				Transformations.rotation.y = 0.0f;
		}
	}
}
