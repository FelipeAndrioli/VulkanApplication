#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Assets {
	struct MeshMaterialData {
		glm::vec4 Ambient = glm::vec4(1.0f);						// ignore w
		glm::vec4 Diffuse = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);		// ignore w
		glm::vec4 Specular = glm::vec4(1.0f);						// ignore w
		glm::vec4 Transmittance = glm::vec4(1.0f);					// ignore w
		glm::vec4 Emission = glm::vec4(1.0f);						// ignore w
		glm::vec4 extra[6];

		alignas(4) int Pad2 = 0;
		alignas(4) int Illum = 0;

		alignas(4) int AmbientTextureIndex = 0;
		alignas(4) int DiffuseTextureIndex = 0;
		alignas(4) int SpecularTextureIndex = 0;
		alignas(4) int BumpTextureIndex = 0;
		alignas(4) int RoughnessTextureIndex = 0;
		alignas(4) int MetallicTextureIndex = 0;
		alignas(4) int NormalTextureIndex = 0;

		alignas(4) int ExtraScalar = 0;

		alignas(4) float Shininess = 0.0f;
		alignas(4) float Ior = 0.0f;
		alignas(4) float Dissolve = 0.0f;
		alignas(4) float Roughness = 0.0f;
		alignas(4) float Metallic = 0.0f;
		alignas(4) float Sheen = 0.0f;
		alignas(4) float ClearcoatThickness = 0.0f;
		alignas(4) float ClearcoatRoughness = 0.0f;
		alignas(4) float Anisotropy = 0.0f;
		alignas(4) float AnisotropyRotation = 0.0f;
		//alignas(4) float Pad0 = 0.0f;
	};

	struct Material {
		std::string Name = "Default";
		MeshMaterialData MaterialData = {};
	};
}
