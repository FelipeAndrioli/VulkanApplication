#include "ShadowCamera.h"

namespace Assets {

	void ShadowCamera::UpdateDirectionalLightShadowMatrix(const glm::vec3 lightDirection) {
		ViewMatrix			= glm::lookAt(lightDirection, Position, glm::vec3(0.0f, 1.0f, 0.0f));
		ProjectionMatrix	= glm::ortho(-LightFrustumSize, LightFrustumSize, -LightFrustumSize, LightFrustumSize, Near, Far);
		m_ShadowMatrix		= ProjectionMatrix * ViewMatrix;
	}

	void ShadowCamera::OnUIRender(const char* idCamera) {
		if (ImGui::TreeNode(idCamera)) {
			if (m_ProjectionType == Assets::Camera::ProjectionType::ORTHOGRAPHIC) {
				ImGui::DragFloat("Light Frustum Size", &LightFrustumSize, 0.02f, -40.0f, 40.0f);
			}

			ImGui::DragFloat("Camera Near Clip", &Near, 0.02f, -200.0f, 200.0f);
			ImGui::DragFloat("Camera Far Clip", &Far, 0.02f, -1000.0f, 1000.0f);
			ImGui::DragFloat("Camera Position X", &Position.x, 0.02f);
			ImGui::DragFloat("Camera Position Y", &Position.y, 0.02f);
			ImGui::DragFloat("Camera Position Z", &Position.z, 0.02f);

			int selectedCameraType = m_ProjectionType;

			if (ImGui::Combo("Camera Type", &selectedCameraType, m_CameraTypes, IM_ARRAYSIZE(m_CameraTypes))) {
				m_ProjectionType = static_cast<Assets::Camera::ProjectionType>(selectedCameraType);
			}

			ImGui::TreePop();
		}

	}
}
