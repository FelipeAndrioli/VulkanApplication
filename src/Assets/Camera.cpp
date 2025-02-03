#include "./Camera.h"

#include "../Input/Input.h"

namespace Assets {
	void Camera::Init(glm::vec3 position, float fov, float yaw, float pitch, uint32_t width, uint32_t height) {
		Position = position;
		Fov = fov;
		Yaw = yaw;
		Pitch = pitch;

		Resize(width, height);
		UpdateCameraVectors();
	}

	void Camera::OnUpdate(float t, const InputSystem::Input& input) {

		if (!input.Mouse.RightButtonPressed) {
			m_LastX = input.Mouse.x;
			m_LastY = input.Mouse.y;
			return;
		}

		if (m_FirstMouse) {
			m_LastX = input.Mouse.x;
			m_LastY = input.Mouse.y;
			m_FirstMouse = false;
		}

		m_OffsetX = input.Mouse.x - m_LastX;
		m_OffsetY = m_LastY - input.Mouse.y;

		m_LastX = input.Mouse.x;
		m_LastY = input.Mouse.y;

		Yaw += static_cast<float>(m_OffsetX) * Sensitivity * t;
		Pitch += static_cast<float>(m_OffsetY) * Sensitivity * t;

		if (input.Keys[GLFW_KEY_W].IsDown) {
			Position += Front * MovementSpeed * t;
		}

		if (input.Keys[GLFW_KEY_S].IsDown) {
			Position -= Front * MovementSpeed * t;
		}

		if (input.Keys[GLFW_KEY_D].IsDown) {
			Position += Right * MovementSpeed * t;
		}

		if (input.Keys[GLFW_KEY_A].IsDown) {
			Position -= Right * MovementSpeed * t;
		}
		
		if (input.Keys[GLFW_KEY_Q].IsDown) {
			Position -= WorldUp * MovementSpeed * t;
		}
		
		if (input.Keys[GLFW_KEY_E].IsDown) {
			Position += WorldUp * MovementSpeed * t;
		}

		UpdateCameraVectors();
		UpdateProjectionMatrix();
	}

	void Camera::OnUIRender(const char* idCamera) {

		if (ImGui::TreeNode(idCamera)) {
			ImGui::DragFloat("Camera Near Clip", &Near, 0.02f, -1.0f, 20.0f);
			ImGui::DragFloat("Camera Far Clip", &Far, 0.02f, 10.0f, 1000.0f);
			ImGui::DragFloat("Camera Position X", &Position.x, 0.02f);
			ImGui::DragFloat("Camera Position Y", &Position.y, 0.02f);
			ImGui::DragFloat("Camera Position Z", &Position.z, 0.02f);
			ImGui::DragFloat("Camera Yaw", &Yaw, 0.02f, -200.0f, 200.0f);
			ImGui::DragFloat("Camera Pitch", &Pitch, 0.02f, -89.0f, 89.0f);
			ImGui::DragFloat("Camera Movement Speed", &MovementSpeed, 0.02f);
			ImGui::DragFloat("Sensitivity", &Sensitivity, 0.02f);
			ImGui::DragFloat("Camera FOV", &Fov, 0.02f);
			ImGui::TreePop();
		}

		UpdateCameraVectors();
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
	}

	void Camera::UpdateViewMatrix() {
		ViewMatrix = glm::lookAt(Position, Position + Front, Up);
	}

	void Camera::UpdateProjectionMatrix() {
		ProjectionMatrix = glm::perspective(glm::radians(Fov), m_Width / (float) m_Height, Near, Far);
		ProjectionMatrix[1][1] *= -1;
	}

	void Camera::Resize(uint32_t width, uint32_t height) {
		m_Width = width;
		m_Height = height;

		UpdateProjectionMatrix();
	}
}
