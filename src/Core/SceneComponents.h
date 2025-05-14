#pragma once

// TODO: Reorganize code directory, create one directory for Graphics only
// TODO: Remove raw cutoff/outer cut off angle, keep only outer/cutoff
// TODO: Add better naming to light properties

#include <glm.hpp>

namespace Scene {

	struct LightComponent {
		enum LightType {
			DIRECTIONAL = 0,
			POINT,
			SPOT	
		};

		glm::vec4 position						= glm::vec4(0.0f);
		glm::vec4 direction						= glm::vec4(0.0f);
		glm::vec4 color							= glm::vec4(1.0f);			// w -> light intensity
		glm::vec4 extra[2]						= {};

		glm::mat4 model							= glm::mat4(1.0f);			// light source rendering
		glm::mat4 viewProj						= glm::mat4(1.0f);			// shadow map 

		alignas(4) LightType type				= LightType::POINT;
		alignas(4) int index					= 0;

		alignas(4) float outerCutOffAngle		= 0.0f;
		alignas(4) float cutOffAngle			= 0.0f;		
		alignas(4) float rawCutOffAngle			= 0.0f;
		alignas(4) float rawOuterCutOffAngle	= 0.0f;
		alignas(4) float linearAttenuation		= 0.0f;
		alignas(4) float quadraticAttenuation	= 0.0f;
		alignas(4) float scale					= 0.0f;
		alignas(4) float ambient				= 0.0f;
		alignas(4) float diffuse				= 0.0f;
		alignas(4) float specular				= 0.0f;

	};
}
