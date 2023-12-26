#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <unordered_map>

#include "../src/Vulkan.h"

namespace Engine {
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class SwapChain;
	class DepthBuffer;
	class Material;

	namespace InputSystem {
		class Input;
	}
}

namespace Assets {

	class Camera;
	class Object;
	struct Vertex;

	class Scene {
	public:
		// will leave constructor/destructor empty for now
		Scene();
		~Scene();

		void AddObject(Object* object);
		void AddMaterial(Engine::Material* material);

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
		std::vector<Object*> Objects;
		std::unordered_map<std::string, Engine::Material*> MapMaterials;
		std::vector<VkRenderPassBeginInfo*> RenderPassBeginInfo;
	
		Camera* MainCamera = nullptr;
	private:
		std::vector<Vertex> m_HelperVertices;
		std::vector<uint16_t> m_HelperIndices;

		Engine::Material* DefaultMaterial = nullptr;

		uint32_t m_Width = 800;
		uint32_t m_Height = 600;
	private:
		void SetObjectResources(Engine::Material* material, Engine::PhysicalDevice& physicalDevice, 
			Engine::LogicalDevice& logicalDevice, Engine::CommandPool& commandPool);
	};
}
