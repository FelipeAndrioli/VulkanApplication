#include "SceneComponents.h"

namespace Scene {
	inline float Max(float A, float B) {
		return A > B ? A : B;
	}

	float CalculateLightRadius(const LightComponent& light) {
		float constant = 1.0f;
		float linear = light.linearAttenuation;
		float quadratic = light.quadraticAttenuation;
		float lightMaxIntensity = Max(Max(light.color.r, light.color.g), light.color.b);
		float radius = (-linear + glm::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * lightMaxIntensity))) / (2 * quadratic);

		return radius;
	}
	
}
