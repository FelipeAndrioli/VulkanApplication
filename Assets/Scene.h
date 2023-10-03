#pragma once

#include <iostream>
#include <vector>

#include "Model.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"
#include "../src/LogicalDevice.h"
#include "../src/PhysicalDevice.h"
#include "../src/DescriptorSetLayout.h"
#include "../src/DescriptorPool.h"

namespace Assets {
	class Scene {
	public:
		// will leave constructor/destructor empty for now
		Scene();
		~Scene();

		virtual void OnCreate() = 0;
		virtual void OnUIRender() = 0;
		virtual void OnUpdate(float time) = 0;

		struct Uniforms;
		struct UBO;

		void AddModel(Model* model);
		void SetupScene(Engine::LogicalDevice* logicalDevice, Engine::PhysicalDevice* physicalDevice, 
			Engine::DescriptorPool* descriptorPool, Engine::DescriptorSetLayout* descriptorSetLayout);

		inline std::vector<Model*>& GetSceneModels() { return m_SceneModels; };
		inline std::vector<Vertex>& GetSceneVertices() { return m_SceneVertices; };
		inline std::vector<uint16_t>& GetSceneIndices() { return m_SceneIndices; };
	private:
		std::vector<Model*> m_SceneModels;
		std::vector<Vertex> m_SceneVertices;
		std::vector<uint16_t> m_SceneIndices;
	};
}
