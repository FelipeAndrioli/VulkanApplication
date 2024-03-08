#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Assets {
	struct MeshMaterialData {
		/*
		glm::vec3 Ambient = glm::vec3(1.0f);
		glm::vec3 Diffuse = glm::vec3(1.0f);
		glm::vec3 Specular = glm::vec3(1.0f);
		glm::vec3 Transmittance = glm::vec3(1.0f);
		glm::vec3 Emission = glm::vec3(1.0f);

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
		//float Pad0 = 0.0f;
		
		alignas(4) int Pad2 = 0;
		alignas(4) int Illum = 0;
		*/

		alignas(4) int AmbientTextureIndex = 0;
		alignas(4) int DiffuseTextureIndex = 0;
		alignas(4) int SpecularTextureIndex = 0;
		alignas(4) int BumpTextureIndex = 0;
		alignas(4) int RoughnessTextureIndex = 0;
		alignas(4) int MetallicTextureIndex = 0;
		alignas(4) int NormalTextureIndex = 0;

		alignas(4) int ExtraScalar = 0;
		glm::vec4 extra[14];
	};

	struct Material {
		std::string Name = "";
		size_t Index = 0;

		MeshMaterialData MaterialData = {};
	};
}
