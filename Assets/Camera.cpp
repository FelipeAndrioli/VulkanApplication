#include "./Camera.h"

namespace Assets {
	Camera::Camera(glm::vec3 position, float fov) : Position(position), Fov(fov) {
		UpdateCameraVectors();
	}

	Camera::~Camera() {

	}

	void Camera::UpdateCameraVectors() {
		Front = glm::normalize(Position - glm::vec3(0.0f, 0.0f, 0.0f));
		Right = glm::normalize(glm::cross(WorldUp, Front));
		Up = glm::normalize(glm::cross(Front, Right));

		UpdateViewMatrix();
	}

	void Camera::UpdateViewMatrix() {
		ViewMatrix = glm::lookAt(Position, Position + Front, Up);
	}
}
