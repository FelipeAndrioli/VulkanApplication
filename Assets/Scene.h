#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>

#include "Model.h"
#include "Camera.h"

#include "../src/Buffer.h"
#include "../src/DescriptorSets.h"
#include "../src/LogicalDevice.h"
#include "../src/PhysicalDevice.h"
#include "../src/DescriptorSetLayout.h"
#include "../src/DescriptorPool.h"
#include "../src/ResourceSet.h"

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
		void AddResourceSetLayout(Engine::ResourceSetLayout* resourceSetLayout);

		void OnCreate();
		void OnUIRender();
		void OnUpdate(float t);

		void SetupScene(
			Engine::LogicalDevice& logicalDevice, 
			Engine::PhysicalDevice& physicalDevice, 
			Engine::CommandPool& commandPool, 
			const Engine::SwapChain& swapChain, 
			const Engine::DepthBuffer& depthBuffer,
			const VkRenderPass& renderPass);
		
	public:
		std::vector<Model*> Models;
		std::vector<Engine::ResourceSet*> ResourceSets;
		std::vector<VkRenderPassBeginInfo*> RenderPassBeginInfo;
	
		Camera DefaultCamera;

	private:
		std::vector<Engine::ResourceSetLayout*> m_ResourceSetLayouts;
	};
}
