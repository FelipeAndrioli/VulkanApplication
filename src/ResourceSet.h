#pragma once

#include "Vulkan.h"
#include "GraphicsPipeline.h"
#include "Buffer.h"
#include "BufferHelper.h"
#include "PipelineLayout.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"
#include "SwapChain.h"
#include "ResourceSetLayout.h"
#include "DepthBuffer.h"

#include "../Assets/Scene.h"

namespace Engine {
	class ResourceSet {
	public:

		ResourceSet(ResourceSetLayout* resourceSetLayout, LogicalDevice* logicalDevice, PhysicalDevice* physicalDevice, 
			CommandPool* commandPool, SwapChain* swapChain, DepthBuffer* depthBuffer, const VkRenderPass& renderPass,
			std::vector<Assets::Model*>& models);

		~ResourceSet();
		
		void SetModelResources(std::vector<Assets::Model*>& models);
		void CreateVertexBuffer();
		void CreateIndexBuffer();

		GraphicsPipeline* GetGraphicsPipeline() { return m_GraphicsPipeline.get(); };
	
		inline Buffer* GetVertexBuffers() const { return m_VertexBuffer.get(); };
		inline Buffer* GetIndexBuffers() const { return m_IndexBuffer.get(); };

		inline std::vector<Assets::Vertex>& GetVertices() { return m_Vertices; };
		inline std::vector<uint16_t>& GetIndices() { return m_Indices; };

	public:
		int ResourceSetIndex = -1;
		std::string ID;
	private:
		ResourceSetLayout* p_ResourceSetLayout = nullptr;

		std::unique_ptr<class GraphicsPipeline> m_GraphicsPipeline;
		std::unique_ptr<class Buffer> m_VertexBuffer;
		std::unique_ptr<class Buffer> m_IndexBuffer;

		std::vector<Assets::Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;

		LogicalDevice* p_LogicalDevice = nullptr;
		PhysicalDevice* p_PhysicalDevice = nullptr;
		CommandPool* p_CommandPool = nullptr;
	};
}
