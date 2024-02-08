#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>

#include "../src/Vulkan.h"

namespace Engine {
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class SwapChain;
	class DepthBuffer;
	class GraphicsPipeline;
	class Buffer;

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
		void SetupSceneGeometryBuffer(
			Engine::LogicalDevice& logicalDevice, 
			Engine::PhysicalDevice& physicalDevice, 
			Engine::CommandPool& commandPool
		);

		void OnCreate();
		void OnUIRender();
		virtual void OnUpdate(float t, const Engine::InputSystem::Input& input);
		void OnResize(uint32_t width, uint32_t height);
	public:
		std::vector<Object*> RenderableObjects;
		std::vector<Assets::GraphicsPipeline> SceneGraphicsPipelines;
	
		Camera* MainCamera = nullptr;

		std::unique_ptr<class Engine::Buffer> SceneGeometryBuffer;
		size_t VertexOffset = 0;
	private:
		uint32_t m_Width = 800;
		uint32_t m_Height = 600;
	};
}
