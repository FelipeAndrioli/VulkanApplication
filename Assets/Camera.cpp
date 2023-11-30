#include "./Camera.h"

namespace Assets {
	Camera::Camera(glm::vec3 position, float fov) : Position(position), Fov(fov) {
		UpdateCameraVectors();
	}

	Camera::~Camera() {

	}

	void Camera::OnUpdate() {
		UpdateCameraVectors();
	}

	void Camera::OnUIRender() {
		if (ImGui::TreeNode("Camera")) {
			ImGui::Text("Camera Position: x - %f, y - %f, z - %f", Position.x, Position.y, Position.z);
			ImGui::Text("Camera Up: x - %f, y - %f, z - %f", Up.x, Up.y, Up.z);
			ImGui::Text("Camera Target: x - %f, y - %f, z - %f", Target.x, Target.y, Target.z);
			ImGui::Text("Camera Near Clip: %f", Near);
			ImGui::Text("Camera Far Clip: %f", Far);
			ImGui::SliderFloat("Camera Position X", &Position.x, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Position Y", &Position.y, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Position Z", &Position.z, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Near Clip", &Near, -1.0f, 20.0f);
			ImGui::SliderFloat("Camera Far Clip", &Far, 10.0f, 200.0f);
			ImGui::TreePop();
		}
	}

	void Camera::UpdateCameraVectors() {
		Front = glm::normalize(Position - glm::vec3(0.0f, 0.0f, 0.0f));
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));

		Target = Position + Front;

		UpdateViewMatrix();
		UpdateProjectionMatrix();
	}

	void Camera::UpdateViewMatrix() {
		//ViewMatrix = glm::lookAt(Position, Target, Up);
		ViewMatrix = glm::translate(glm::mat4(1.0f), Position);
	}

	void Camera::UpdateProjectionMatrix() {
		ProjectionMatrix = glm::perspective(glm::radians(Fov), 800 / (float) 600, Near, Far);
		ProjectionMatrix[1][1] *= -1;
	}
}
