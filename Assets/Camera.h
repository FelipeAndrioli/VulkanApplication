#pragma once

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include "../src/Input/Input.h"

namespace Assets {
	class Camera {
	public:
		Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), float fov = 45.0f, 
			uint32_t width = 800, uint32_t height = 600
		);
		~Camera();

		void OnUpdate(float t, const Engine::InputSystem::Input& input);
		void OnUIRender();
		void UpdateCameraVectors();
		void Resize(uint32_t width, uint32_t height);
		void UpdateViewMatrix();
		void UpdateProjectionMatrix();
	public:
		float Fov;
		float Near = 0.1f;
		float Far = 200.0f;
		float Yaw = -90.0f;
		float Pitch = 0.0f;

		float MovementSpeed = 0.01f;
		float Sensitivity = 0.1f;

		glm::mat4 ViewMatrix = glm::mat4(1.0f);
		glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
	
		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 Up;
		glm::vec3 Right;
	private:
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		double m_LastX = 0.0;
		double m_LastY = 0.0;
		double m_OffsetX = 0.0;
		double m_OffsetY = 0.0;
		
		bool m_FirstMouse = true;
	};
}
