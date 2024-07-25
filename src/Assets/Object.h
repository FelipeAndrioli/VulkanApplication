#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <gtc/matrix_transform.hpp>

#include "../Graphics.h"

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

		virtual void OnCreate() {};
		virtual void OnUpdate(float t);
		virtual void OnUIRender();

		glm::mat4 GetModelMatrix();

		void ResetResources();
		void AddMeshes(std::vector<Assets::Mesh> meshes);
		void AddMesh(Assets::Mesh mesh);
		//void SetGraphicsPipeline(Engine::GraphicsPipeline* graphicsPipeline) { m_GraphicsPipeline = graphicsPipeline; }
		//Engine::GraphicsPipeline* GetGraphicsPipeline() { return m_GraphicsPipeline; }

	public:
		std::string ID = "";
		std::string PipelineName = "";

		const char* ModelPath = nullptr;
		const char* MaterialPath = nullptr;

		bool Textured = false;
		bool FlipTexturesVertically = false;
		bool GenerateMipMaps = true;
		bool m_Rotate = false;

		std::vector<Mesh> Meshes;

		Transform Transformations = {};

		//std::unique_ptr<class Engine::DescriptorSets> DescriptorSets[Engine::MAX_FRAMES_IN_FLIGHT];
		//VkDescriptorSet DescriptorSets[FRAMES_IN_FLIGHT];

		uint32_t IndicesAmount = 0;
		uint32_t VerticesAmount = 0;
	private:
		//Engine::GraphicsPipeline* m_GraphicsPipeline = nullptr;
	};
}
