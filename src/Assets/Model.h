#pragma once

#include <gtc/matrix_transform.hpp>

#include "Mesh.h"

#include "../Core/Graphics.h"

namespace Assets {
	struct Transform {
		glm::vec3 translation;
		glm::vec3 rotation;

		float scaleHandler;
	};

	class Model {
	public:

		~Model();

		virtual void OnCreate() {};
		virtual void OnUpdate(float t);
		virtual void OnUIRender();
		void Destroy();

		glm::mat4 GetModelMatrix();

	public:
		std::string Name = "";

		std::vector<Assets::Mesh> Meshes;
		std::vector<Graphics::Texture> Textures;

		Transform Transformations = {};

		size_t TotalVertices = 0;
		size_t TotalIndices = 0;

		bool FlipUvVertically = false;
		bool GenerateMipMaps = true;
		bool Rotate = false;

		std::string ModelPath;
		std::string MaterialPath;

		Graphics::GPUBuffer DataBuffer = {};
		Graphics::Buffer ModelBuffer = {};

		VkDescriptorSetLayout ModelDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet ModelDescriptorSet = VK_NULL_HANDLE;
	private:
		Graphics::GPUBuffer m_MaterialBuffer = {};
	};
}
