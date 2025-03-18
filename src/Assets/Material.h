#pragma once

#include <string>
#include <glm.hpp>

struct MaterialData {
	glm::vec4 Ambient = glm::vec4(0.5f);						// ignore w
	glm::vec4 Diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);		// ignore w
	glm::vec4 Specular = glm::vec4(0.1f);						// ignore w
	glm::vec4 Transparency = glm::vec4(1.0f);					// ignore w
	glm::vec4 Emission = glm::vec4(1.0f);						// ignore w
	glm::vec4 extra[6];

	alignas(4) int Pad2 = 0;
	alignas(4) int Illum = 0;

	alignas(4) int AmbientTextureIndex = -1;
	alignas(4) int DiffuseTextureIndex = -1;
	alignas(4) int SpecularTextureIndex = -1;
	alignas(4) int BumpTextureIndex = -1;
	alignas(4) int RoughnessTextureIndex = -1;
	alignas(4) int MetallicTextureIndex = -1;
	alignas(4) int NormalTextureIndex = -1;

	alignas(4) int ExtraScalar = 0;

	alignas(4) float Opacity = 0.0f;
	alignas(4) float Shininess = 0.0f;
	alignas(4) float ShininessStrength = 0.0f;
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
	Material() {};

	Material(std::string materialName, MaterialData materialData) {
		Name = materialName;
		MaterialData = materialData;
	}

	Material(std::string materialName) {
		Name = materialName;
	}
	
	Material(MaterialData materialData) {
		MaterialData = materialData;
	}

	std::string Name = "Default";
	MaterialData MaterialData = {};
};
