#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <gtc/matrix_transform.hpp>

namespace Engine {
	class Buffer;
	class DescriptorSets;
	class Image;
	class GraphicsPipeline;
}

namespace Assets {
	struct Mesh;

	struct Transform {
		glm::vec3 translation;
		glm::vec3 rotation;

		float scaleHandler;
	};

	class Object {
	public:
		Object(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f));
		~Object();

		virtual void OnCreate() = 0;
		virtual void OnUpdate(float t) = 0;
		virtual void OnUIRender() = 0;

		glm::mat4 GetModelMatrix();

		void ResetResources();
		void SetMesh(std::vector<Assets::Mesh> mesh);
		void SetGraphicsPipeline(Engine::GraphicsPipeline* graphicsPipeline) { m_GraphicsPipeline = graphicsPipeline; }
		Engine::GraphicsPipeline* GetGraphicsPipeline() { return m_GraphicsPipeline; }

	public:
		std::string ID = "";
		std::string PipelineName = "";
		const char* ModelPath = nullptr;
		const char* MaterialPath = nullptr;
		bool Textured = false;
		bool FlipTexturesVertically = false;
		bool GenerateMipMaps = true;
		std::vector<Mesh> Meshes;
		Transform Transformations = {};
		std::unique_ptr<class Engine::DescriptorSets> DescriptorSets;

	private:
		Engine::GraphicsPipeline* m_GraphicsPipeline = nullptr;
	};
}
