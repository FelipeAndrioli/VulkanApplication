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

	void ShadowCamera::UpdateOmniDirectionalShadowMatrix(const glm::vec3& lightPosition) {

		// Note:	Aspect ratio of a omnidirectional shadow matrix must always be 1.0 because it renders to a cube (a cube must
		//			have width equals to the height, d'oh!), otherwise the shadow will be misplaced or distorted.

		ProjectionMatrix = glm::perspective(glm::radians(PointLightFov), 1.0f, PointLightNear, PointLightFar);
		
		// Positive X
		OmniViewMatrix[0] = glm::mat4(1.0f);
		OmniViewMatrix[0] = glm::lookAt(lightPosition, lightPosition + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

		// Negative X
		OmniViewMatrix[1] = glm::mat4(1.0f);
		OmniViewMatrix[1] = glm::lookAt(lightPosition, lightPosition + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

		// Positive Y
		OmniViewMatrix[2] = glm::mat4(1.0f);
		OmniViewMatrix[2] = glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		
		// Negative Y
		OmniViewMatrix[3] = glm::mat4(1.0f);
		OmniViewMatrix[3] = glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

		// Positive Z
		OmniViewMatrix[4] = glm::mat4(1.0f);
		OmniViewMatrix[4] = glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		
		// Negative Z
		OmniViewMatrix[5] = glm::mat4(1.0f);
		OmniViewMatrix[5] = glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	}

	void ShadowCamera::OnUIRender(const char* idCamera) {
		if (ImGui::TreeNode(idCamera)) {
			ImGui::DragFloat("Dir Light Frustum Size",	&m_DirLightFrustumSize, 0.02f, -40.0f, 40.0f);
			ImGui::DragFloat("Dir Light Near",			&m_DirLightNear, 0.02f, -1000.0f, 1000.0f);
			ImGui::DragFloat("Dir Light Far",			&m_DirLightFar, 0.02f, -1000.0f, 1000.0f);

			ImGui::DragFloat("Spot Light FOV",	&m_SpotLightFov, 0.02f, 0.0f, 200.0f);
			ImGui::DragFloat("Spot Light Near", &m_SpotLightNear, 0.02f, 0.0f, 200.0f);
			ImGui::DragFloat("Spot Light Far",	&m_SpotLightFar, 0.02f, 0.0f, 200.0f);

			ImGui::DragFloat("Point Light Near", &PointLightNear, 0.02f, -1000.0f, 1000.0f);
			ImGui::DragFloat("Point Light Far", &PointLightFar, 0.02f, -1000.0f, 1000.0f);
			ImGui::DragFloat("Point Light FOV", &PointLightFov, 0.02f, -200.0f, 200.0f);

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

	void ShadowCamera::SetPointLightSettings(float near, float far) {
		PointLightNear	= near;
		PointLightFar	= far;
	}
}
