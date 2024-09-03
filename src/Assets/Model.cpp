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
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::scale(model, glm::vec3(Transformations.scaleHandler, Transformations.scaleHandler, Transformations.scaleHandler));
		model = glm::translate(model, Transformations.translation);
		model = glm::rotate(model, glm::radians(Transformations.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Transformations.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(Transformations.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		return model;
	}


	void Model::OnUIRender() {
		std::string t = "Transformations " + Name;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + Name;
			std::string t_label_y = "Translation y " + Name;
			std::string t_label_z = "Translation z " + Name;

			ImGui::SliderFloat(t_label_x.c_str(), &Transformations.translation.x, -200.0f, 200.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &Transformations.translation.y, -200.0f, 200.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &Transformations.translation.z, -200.0f, 200.0f);

			std::string r_label_x = "Rotation x " + Name;
			std::string r_label_y = "Rotation y " + Name;
			std::string r_label_z = "Rotation z " + Name;

			ImGui::SliderFloat(r_label_x.c_str(), &Transformations.rotation.x, -200.0f, 200.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &Transformations.rotation.y, -200.0f, 200.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &Transformations.rotation.z, -200.0f, 200.0f);

			std::string s_label = "Scale Handler " + Name;
			ImGui::SliderFloat(s_label.c_str(), &Transformations.scaleHandler, 0.0f, 20.0f);

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
