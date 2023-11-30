#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

namespace Assets {
	class Camera {
	public:
		Camera(
			glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), float fov = 45.0f);
		~Camera();

		void OnUpdate();
		void OnUIRender();
		void UpdateCameraVectors();
	public:
		float Fov;
		float Near = 0.1f;
		float Far = 200.0f;

		glm::mat4 ViewMatrix = glm::mat4(1.0f);
		glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
		
		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 Up;
		glm::vec3 Right;
		glm::vec3 Target;

	private:
		void UpdateViewMatrix();
		void UpdateProjectionMatrix();
	};
}
