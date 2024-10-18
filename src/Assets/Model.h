#pragma once

#include <gtc/matrix_transform.hpp>

#include "Mesh.h"

#include "../Core/Graphics.h"

namespace Renderer {
	class MeshSorter;
};

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
		virtual void Render(Renderer::MeshSorter& sorter);
		void Destroy();

		glm::mat4 GetModelMatrix();

		void AddPipelineFlag(uint16_t flag);
		void RemovePipelineFlag(uint16_t flag);
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
		bool RenderOutline = false;
		bool FirstStencil = true;
		bool StencilTest = false;

		float OutlineWidth = 0.0f;

		uint32_t ModelIndex = 0;

		std::string ModelPath;
		std::string MaterialPath;

		Graphics::GPUBuffer DataBuffer = {};
		Graphics::Buffer ModelBuffer = {};

		VkDescriptorSetLayout ModelDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet ModelDescriptorSet = VK_NULL_HANDLE;

		glm::vec3 PivotVector = glm::vec3(1.0f);
	};
}
