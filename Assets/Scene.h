#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>

#include "Model.h"

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

		void SetupScene(Engine::LogicalDevice* logicalDevice, Engine::PhysicalDevice* physicalDevice, 
			Engine::CommandPool* commandPool, Engine::SwapChain* swapChain, Engine::DepthBuffer* depthBuffer,
			const VkRenderPass& renderPass);
		
		void Resize(Engine::SwapChain* swapChain, Engine::DepthBuffer* depthBuffer);
	private:
		//void CreateRenderPassBeginInfo(Engine::SwapChain* swapChain);
	public:
		std::vector<Model*> Models;
		std::vector<Engine::ResourceSet*> ResourceSets;
		std::vector<VkRenderPassBeginInfo*> RenderPassBeginInfo;

		// TODO: temporary here, must move it back to private once I figure out the issue with RenderPassBeginInfo
		std::vector<Engine::ResourceSetLayout*> m_ResourceSetLayouts;
	private:
	};
}
