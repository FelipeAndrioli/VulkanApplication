#include "./Camera.h"

namespace Assets {
	Camera::Camera(glm::vec3 position, float fov, uint32_t width, uint32_t height) : Position(position), Fov(fov) {
		Resize(width, height);
		UpdateCameraVectors();
	}

	Camera::~Camera() {

	}

	void Camera::OnUpdate(float t, const Engine::InputSystem::Input& input) {

		if (!input.Mouse.LeftButtonPressed) {
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

		if (input.Keys[GLFW_KEY_W].IsPressed) {
			Position += Front * MovementSpeed * t;
		}

		if (input.Keys[GLFW_KEY_S].IsPressed) {
			Position -= Front * MovementSpeed * t;
		}

		if (input.Keys[GLFW_KEY_D].IsPressed) {
			Position += Right * MovementSpeed * t;
		}

		if (input.Keys[GLFW_KEY_A].IsPressed) {
			Position -= Right * MovementSpeed * t;
		}
		
		if (input.Keys[GLFW_KEY_Q].IsPressed) {
			Position -= WorldUp * MovementSpeed * t;
		}
		
		if (input.Keys[GLFW_KEY_E].IsPressed) {
			Position += WorldUp * MovementSpeed * t;
		}

		UpdateCameraVectors();
		UpdateProjectionMatrix();
	}

	void Camera::OnUIRender() {
		if (ImGui::TreeNode("Camera")) {
			ImGui::SliderFloat("Camera Near Clip", &Near, -1.0f, 20.0f);
			ImGui::SliderFloat("Camera Far Clip", &Far, 10.0f, 200.0f);
			ImGui::SliderFloat("Camera Position X", &Position.x, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Position Y", &Position.y, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Position Z", &Position.z, -20.0f, 20.0f);
			ImGui::SliderFloat("Camera Yaw", &Yaw, -200.0f, 200.0f);
			ImGui::SliderFloat("Camera Pitch", &Pitch, -89.0f, 89.0f);
			ImGui::SliderFloat("Camera Movement Speed", &MovementSpeed, 0.0f, 0.10f);
			ImGui::SliderFloat("Sensitivity", &Sensitivity, 0.0f, 1.0f);
			ImGui::SliderFloat("Camera FOV", &Fov, 0.0f, 100.0f);
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
