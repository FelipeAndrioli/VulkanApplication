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
			ImGui::Text("Camera Near Clip: %f", Near);
			ImGui::Text("Camera Far Clip: %f", Far);
			ImGui::Text("Camera Position: x - %f, y - %f, z - %f", Position.x, Position.y, Position.z);
			ImGui::Text("Camera Up: x - %f, y - %f, z - %f", Up.x, Up.y, Up.z);
			ImGui::Text("Camera Yaw: %f", Yaw);
			ImGui::Text("Camera Pitch: %f", Pitch);
			ImGui::SliderFloat("Camera Near Clip", &Near, -1.0f, 20.0f);
			ImGui::SliderFloat("Camera Far Clip", &Far, 10.0f, 200.0f);
			ImGui::SliderFloat("Camera Position X", &Position.x, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Position Y", &Position.y, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Position Z", &Position.z, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Yaw", &Yaw, -200.0f, 200.0f);
			ImGui::SliderFloat("Camera Pitch", &Pitch, -89.0f, 89.0f);
			ImGui::TreePop();
		}
	}

	void Camera::UpdateCameraVectors() {

		if (Pitch > 89.0f) Pitch = 89.0f;
		if (Pitch < -89.0f) Pitch = -89.0f;

		glm::vec3 front = glm::vec3(1.0f);

		front.x = glm::cos(glm::radians(Yaw)) * glm::cos(glm::radians(Pitch));
		front.y = glm::sin(glm::radians(Pitch));
		front.z = glm::sin(glm::radians(Yaw)) * glm::cos(glm::radians(Pitch));

		Front = glm::normalize(front);
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));

		UpdateViewMatrix();
		UpdateProjectionMatrix();
	}

	void Camera::UpdateViewMatrix() {
		ViewMatrix = glm::lookAt(Position, Position + Front, Up);
	}

	void Camera::UpdateProjectionMatrix() {
		ProjectionMatrix = glm::perspective(glm::radians(Fov), 800 / (float) 600, Near, Far);
		ProjectionMatrix[1][1] *= -1;
	}
}
