#include "ShadowCamera.h"

namespace Assets {

	void ShadowCamera::UpdateDirectionalLightShadowMatrix(const glm::vec3& lightDirection) {
		ViewMatrix			= glm::lookAt(lightDirection, Position, glm::vec3(0.0f, 1.0f, 0.0f));
		ProjectionMatrix	= glm::ortho(-m_DirLightFrustumSize, m_DirLightFrustumSize, -m_DirLightFrustumSize, m_DirLightFrustumSize, m_DirLightNear, m_DirLightFar);
		m_ShadowMatrix		= ProjectionMatrix * ViewMatrix;
	}

	void ShadowCamera::UpdateSpotLightShadowMatrix(const glm::vec3& lightPosition, const glm::vec3& lightDirection) {
		ViewMatrix			= glm::lookAt(lightPosition, lightPosition - (-lightDirection), WorldUp);
		ProjectionMatrix	= glm::perspective(glm::radians(m_SpotLightFov), m_Width / (float)m_Height, m_SpotLightNear, m_SpotLightFar);
		m_ShadowMatrix		= ProjectionMatrix * ViewMatrix;
	}

	void ShadowCamera::OnUIRender(const char* idCamera) {
		if (ImGui::TreeNode(idCamera)) {
			ImGui::DragFloat("Dir Light Frustum Size",	&m_DirLightFrustumSize, 0.02f, -40.0f, 40.0f);
			ImGui::DragFloat("Dir Light Near",			&m_DirLightNear, 0.02f, -1000.0f, 1000.0f);
			ImGui::DragFloat("Dir Light Far",			&m_DirLightFar, 0.02f, -1000.0f, 1000.0f);

			ImGui::DragFloat("Spot Light FOV",	&m_SpotLightFov, 0.02f, 0.0f, 200.0f);
			ImGui::DragFloat("Spot Light Near", &m_SpotLightNear, 0.02f, 0.0f, 200.0f);
			ImGui::DragFloat("Spot Light Far",	&m_SpotLightFar, 0.02f, 0.0f, 200.0f);

			ImGui::DragFloat("Camera Position X",	&Position.x,	0.02f);
			ImGui::DragFloat("Camera Position Y",	&Position.y,	0.02f);
			ImGui::DragFloat("Camera Position Z",	&Position.z,	0.02f);
			
			ImGui::TreePop();
		}

	}

	void ShadowCamera::Resize(uint32_t width, uint32_t height) {
		m_Width		= width;
		m_Height	= height;
	}

	void ShadowCamera::SetSpotLightSettings(float near, float far, float fov) {
		m_SpotLightNear = near;
		m_SpotLightFar	= far;
		m_SpotLightFov	= fov;
	}

	void ShadowCamera::SetDirLightSettings(float near, float far, float frustumSize) {
		m_DirLightNear			= near;
		m_DirLightFar			= far;
		m_DirLightFrustumSize	= frustumSize;
	}
}
