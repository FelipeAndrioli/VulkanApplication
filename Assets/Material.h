#pragma once

#include <string>
#include <memory>
#include <map>

#include <glm/glm.hpp>

#include "./Texture.h"

namespace Assets {
	struct Texture;
	struct TextureIndices;
	enum TextureType;

	struct MaterialProperties {
		std::string Name = "";

		glm::vec3 Ambient = glm::vec3(1.0f);
		glm::vec3 Diffuse = glm::vec3(1.0f);
		glm::vec3 Specular = glm::vec3(1.0f);
		glm::vec3 Transmittance = glm::vec3(1.0f);
		glm::vec3 Emission = glm::vec3(1.0f);

		float Shininess = 0.0f;
		float Ior = 0.0f;
		float Dissolve = 0.0f;
		float Roughness = 0.0f;
		float Metallic = 0.0f;
		float Sheen = 0.0f;
		float ClearcoatThickness = 0.0f;
		float ClearcoatRoughness = 0.0f;
		float Anisotropy = 0.0f;
		float AnisotropyRotation = 0.0f;
		//float Pad0 = 0.0f;
		
		int Pad2 = 0;
		int Illum = 0;
	};

	struct Material {
		MaterialProperties Properties = {};
		size_t Index = 0;
		Assets::TextureIndices MaterialTextureIndices;
	};
}
