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
#include "MaterialLayout.h"
#include "DepthBuffer.h"

namespace Engine {
	class Material {
	public:

		Material();
		~Material();
	
		void Init(LogicalDevice& logicalDevice,
			PhysicalDevice& physicalDevice,
			CommandPool& commandPool,
			const SwapChain& swapChain,
			const DepthBuffer& depthBuffer,
			const VkRenderPass& renderPass
		);

		GraphicsPipeline* GetGraphicsPipeline() { return m_GraphicsPipeline.get(); };

		inline Buffer* GetVertexBuffers() const { return m_VertexBuffer.get(); };
		inline Buffer* GetIndexBuffers() const { return m_IndexBuffer.get(); };

		void CreateVertexBuffer(std::vector<Assets::Vertex> vertices, PhysicalDevice& physicalDevice, 
			LogicalDevice& logicalDevice, CommandPool& commandPool);

		void CreateIndexBuffer(std::vector<uint16_t> indices, PhysicalDevice& physicalDevice, 
			LogicalDevice& logicalDevice, CommandPool& commandPool);
	public:
		MaterialLayout Layout = MaterialLayout();
	private:
		std::unique_ptr<class GraphicsPipeline> m_GraphicsPipeline;
		
		std::unique_ptr<class Buffer> m_VertexBuffer;
		std::unique_ptr<class Buffer> m_IndexBuffer;
	};
}
