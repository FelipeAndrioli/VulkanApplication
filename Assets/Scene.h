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
#include "../src/Material.h"

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
		void AddMaterial(Engine::Material* material);

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
		std::unordered_map<std::string, Engine::Material*> MapMaterials;
		std::vector<VkRenderPassBeginInfo*> RenderPassBeginInfo;
	
		Camera DefaultCamera;
	private:
		std::vector<Vertex> m_HelperVertices;
		std::vector<uint16_t> m_HelperIndices;

		Engine::Material* DefaultMaterial = nullptr;

	private:
		void SetModelResources(Engine::Material* material, Engine::PhysicalDevice& physicalDevice, 
			Engine::LogicalDevice& logicalDevice);
	};
}
