#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <string>

#include "../src/Vulkan.h"

namespace Engine {
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class SwapChain;
	class DepthBuffer;
	class Material;
	class GraphicsPipeline;

	namespace InputSystem {
		class Input;
	}
}

namespace Assets {

	class Camera;
	class Object;
	class GraphicsPipeline;

	struct Vertex;

	class Scene {
	public:
		// will leave constructor/destructor empty for now
		Scene();
		~Scene();

		void AddRenderableObject(Object* object);
		void AddGraphicsPipeline(Assets::GraphicsPipeline newPipeline);

		void OnCreate();
		void OnUIRender();
		virtual void OnUpdate(float t, const Engine::InputSystem::Input& input);
		void OnResize(uint32_t width, uint32_t height);

		void SetupScene(
			Engine::LogicalDevice& logicalDevice, 
			Engine::PhysicalDevice& physicalDevice, 
			Engine::CommandPool& commandPool, 
			const Engine::SwapChain& swapChain, 
			const Engine::DepthBuffer& depthBuffer,
			const VkRenderPass& renderPass);
		
	public:
		std::vector<Object*> RenderableObjects;
		std::vector<Assets::GraphicsPipeline> GraphicsPipelinesData;
		std::map<std::string, std::unique_ptr<class Engine::GraphicsPipeline>> GraphicsPipelines;
		std::unique_ptr<std::map<std::string, std::unique_ptr<Engine::Material>>> Materials;
		std::vector<VkRenderPassBeginInfo*> RenderPassBeginInfo;
	
		Camera* MainCamera = nullptr;
	private:
		uint32_t m_Width = 800;
		uint32_t m_Height = 600;
	};
}
