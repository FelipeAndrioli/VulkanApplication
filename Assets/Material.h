#pragma once

#include <string>
#include <memory>
#include <map>

#include <glm/glm.hpp>

namespace Engine {
	class Buffer;
	class DescriptorSets;
}

namespace Assets {
	struct Texture;
	enum TextureType;

	class Material {
	public:

		Material();
		~Material();

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

	public:
		MaterialProperties Properties = {};

		size_t Index = 0;

		std::map<Assets::TextureType, Assets::Texture*> Textures;
		std::unique_ptr<class Engine::DescriptorSets> DescriptorSets;
	};
}
