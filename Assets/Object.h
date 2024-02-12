#pragma once

#include <iostream>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"

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
		Object();
		~Object();

		virtual void OnCreate() = 0;
		virtual void OnUpdate(float t) = 0;
		virtual void OnUIRender() = 0;

		glm::mat4 GetModelMatrix();

		void ResetResources();
	public:
		std::string ID = "";
		std::string PipelineName = "";
		const char* ModelPath = nullptr;
		const char* MaterialPath = nullptr;
		bool Textured = false;
		bool FlipTexturesVertically = false;
		std::vector<Mesh*> Meshes;
		Engine::GraphicsPipeline* SelectedGraphicsPipeline = nullptr;
		Transform Transformations = {};
		std::unique_ptr<class Engine::DescriptorSets> DescriptorSets;
	};
}
