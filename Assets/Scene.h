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
#include "../src/ResourceSet.h"
#include "../src/RenderLayout.h"

namespace Engine {
	class ResourceSet;
}

namespace Assets {

	class Scene {
	public:
		// will leave constructor/destructor empty for now
		Scene();
		~Scene();


		void AddModel(Model* model);
		void SetupScene(Engine::RenderLayout* renderLayout, Engine::LogicalDevice* logicalDevice,
			Engine::PhysicalDevice* physicalDevice, Engine::CommandPool* commandPool, Engine::SwapChain* swapChain);
		void Resize();

		const std::vector<Model*>& GetModels() const { return p_Models; };

		inline std::vector<Engine::ResourceSet*> GetResourceSets() { return m_ResourceSets; };
		Engine::ResourceSet* GetResourceSet(size_t index) { return m_ResourceSets[index]; };
		
		void OnCreate();
		void OnUIRender();
		void OnUpdate(float t);
	private:
		std::vector<Engine::ResourceSet*> m_ResourceSets;
		std::vector<Model*> p_Models;
	};
}
