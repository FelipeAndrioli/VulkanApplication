#include "Model.h"

#include <iostream>

#include "../Core/GraphicsDevice.h"
#include "../Core/UI.h"

#include "../Renderer.h"

#include "Camera.h"

namespace Assets {
	Model::~Model() {
		std::cout << "Destroying model " << Name << '\n';
		Destroy();
	}

	void Model::Destroy() {
		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

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

		if (ImGui::TreeNode(Name.c_str())) {
			if (ImGui::TreeNode("Transformations")) {
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

				ImGui::TreePop();
			}
			
			ImGui::Checkbox("Flip UV Vertically", &FlipUvVertically);
			ImGui::Checkbox("Render Outline", &RenderOutline);

			if (RenderOutline) {
				ImGui::DragFloat("Outline Width", &OutlineWidth, 0.0002f, -5.0f, 5.0f, "%.04f");
			}

			ImGui::Checkbox("Stencil Test", &StencilTest);

			if (StencilTest && FirstStencil) {
				AddPipelineFlag(PSOFlags::tStencilTest);
				FirstStencil = false;
			}

			if (!StencilTest && !FirstStencil) {
				FirstStencil = true;
				RemovePipelineFlag(PSOFlags::tStencilTest);
			}

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

	void Model::Render(Renderer::MeshSorter& sorter) {
		
		for (const auto& mesh : Meshes) {
			glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -mesh.PivotVector);
			glm::mat4 toPosition = glm::translate(glm::mat4(1.0f), Transformations.translation);
			glm::mat4 meshModel = toPosition * toOrigin;

			glm::vec3 transformedMeshPivot = glm::vec3(meshModel * glm::vec4(mesh.PivotVector, 1.0));

			float distance = glm::length(sorter.GetCamera().Position - transformedMeshPivot);

			sorter.AddMesh(mesh, distance, ModelIndex, TotalIndices, DataBuffer);
		}
	}

	void Model::AddPipelineFlag(uint16_t flag) {
		for (auto& mesh : Meshes) {
			mesh.PSOFlags |= flag;
		}
	}

	void Model::RemovePipelineFlag(uint16_t flag) {
		for (auto& mesh : Meshes) {
			mesh.PSOFlags ^= flag;
		}
	}
}
