#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm.hpp>

#include "Camera.h"

namespace Assets {
	class ShadowCamera : public Camera {
	public:
		ShadowCamera(float frustumSize, float near, float far) : LightFrustumSize(frustumSize) {
			Near				= near;
			Far					= far;
			Fov					= 45.0f;
			Position			= glm::vec3(0.0f, 0.0f, 0.0f);
			m_ProjectionType	= Assets::Camera::ProjectionType::ORTHOGRAPHIC;
		}

		ShadowCamera() {
			Near				= 30.0f;
			Far					= -30.0f;
			Fov					= 45.0f;
			Position			= glm::vec3(0.0f, 0.0f, 0.0f);
			m_ProjectionType	= Assets::Camera::ProjectionType::ORTHOGRAPHIC;
		};

		void UpdateDirectionalLightShadowMatrix (const glm::vec3 lightDirection);
		void OnUIRender							(const char* idCamera) override;

		inline const glm::mat4& GetShadowMatrix	() const { return m_ShadowMatrix; }

	public:
		float LightFrustumSize = 20.0f;
	private:
		glm::mat4 m_ShadowMatrix = glm::mat4(1.0f);
	};
}