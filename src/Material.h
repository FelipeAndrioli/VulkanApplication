#pragma once

#include "Vulkan.h"
#include "MaterialLayout.h"
#include "GraphicsPipeline.h"

namespace Engine {
	class GraphicsPipeline;
	class Buffer;
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class SwapChain;
	class DepthBuffer;
	class Image;

	class Material {
	public:

		Material();
		~Material();
	
		void Create(LogicalDevice& logicalDevice,
			PhysicalDevice& physicalDevice,
			CommandPool& commandPool,
			const SwapChain& swapChain,
			const DepthBuffer& depthBuffer,
			const VkRenderPass& renderPass
		);

		GraphicsPipeline* GetGraphicsPipeline() { return m_GraphicsPipeline.get(); };

		inline Buffer* GetVertexBuffers() const { return m_VertexBuffer.get(); };
		inline Buffer* GetIndexBuffers() const { return m_IndexBuffer.get(); };

		void CreateVertexBuffer(
			std::vector<Assets::Vertex> vertices, 
			PhysicalDevice& physicalDevice, 
			LogicalDevice& logicalDevice, 
			CommandPool& commandPool
		);

		void CreateIndexBuffer(
			std::vector<uint32_t> indices, 
			PhysicalDevice& physicalDevice, 
			LogicalDevice& logicalDevice, 
			CommandPool& commandPool
		);

		void LoadTexture(
			PhysicalDevice& physicalDevice, 
			LogicalDevice& logicalDevice, 
			CommandPool& commandPool
		);

	public:
		MaterialLayout Layout = MaterialLayout();
		std::unique_ptr<class Image> Texture;
	private:
		std::unique_ptr<class GraphicsPipeline> m_GraphicsPipeline;
		
		std::unique_ptr<class Buffer> m_VertexBuffer;
		std::unique_ptr<class Buffer> m_IndexBuffer;
	};
}
