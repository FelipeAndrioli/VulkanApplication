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
		glm::vec4 extra							= {};

		glm::mat4 model							= glm::mat4(1.0f);			// light source rendering
		glm::mat4 viewProj						= glm::mat4(1.0f);			// shadow map 

		alignas(4) LightType type				= LightType::POINT;
		alignas(4) uint32_t flags				= (1 << 1);					// flags for all purposes such as enabled features, 1 bit only | stratified disk sampling | pcf | shadow map (enabled by default)
		alignas(4) int index					= 0;
		alignas(4) int pcfSamples				= 0;

		alignas(4) float minBias				= 0.002f;
		alignas(4) float spsSpread				= 0.0f;
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
