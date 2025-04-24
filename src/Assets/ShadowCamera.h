#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm.hpp>

#include "Camera.h"

namespace Assets {
	class ShadowCamera : public Camera {
	public:
		ShadowCamera() {}

		ShadowCamera(uint32_t width, uint32_t height) {
			m_Width		= width;
			m_Height	= height;
		}

		void UpdateDirectionalLightShadowMatrix (const glm::vec3& lightDirection);
		void UpdateSpotLightShadowMatrix		(const glm::vec3& lightPosition, const glm::vec3& lightDirection);
		void OnUIRender							(const char* idCamera)													override;
		void Resize								(uint32_t width, uint32_t height)										override;
		void SetSpotLightSettings				(float near, float far, float fov);
		void SetDirLightSettings				(float near, float far, float frustumSize);

		inline const glm::mat4& GetShadowMatrix	() const { return m_ShadowMatrix; }
	private:
		glm::mat4 m_ShadowMatrix = glm::mat4(1.0f);

		float m_SpotLightNear		= 0.0f;
		float m_SpotLightFar		= 0.0f;
		float m_SpotLightFov		= 0.0f;
		float m_DirLightNear		= 0.0f;
		float m_DirLightFar			= 0.0f;
		float m_DirLightFrustumSize = 0.0f;
	};
}