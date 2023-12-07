#include "Material.h"

namespace Engine {
	Material::Material() {

	}

	Material::~Material() {
		m_GraphicsPipeline.reset();
		m_VertexBuffer.reset();
		m_IndexBuffer.reset();
	}

	void Material::Init(LogicalDevice& logicalDevice, 
		PhysicalDevice& physicalDevice, 
		CommandPool& commandPool, 
		const SwapChain& swapChain, 
		const DepthBuffer& depthBuffer, 
		const VkRenderPass& renderPass
	) {
		m_GraphicsPipeline.reset(new class GraphicsPipeline(Layout, logicalDevice, swapChain, depthBuffer, renderPass));
	}

	void Material::CreateVertexBuffer(std::vector<Assets::Vertex> vertices, PhysicalDevice& physicalDevice, 
		LogicalDevice& logicalDevice, CommandPool& commandPool) {

		VkDeviceSize bufferSize = sizeof(Assets::Vertex) * vertices.size();
	
		m_VertexBuffer.reset(new class Buffer(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT), logicalDevice, physicalDevice, 
			bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
		m_VertexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferHelper bufferHelper;
		bufferHelper.CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(),
			vertices, m_VertexBuffer.get());
	}

	void Material::CreateIndexBuffer(std::vector<uint16_t> indices, PhysicalDevice& physicalDevice, 
		LogicalDevice& logicalDevice, CommandPool& commandPool) {

		VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();

		m_IndexBuffer.reset(new class Buffer(1, logicalDevice, physicalDevice,
			bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
		m_IndexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferHelper bufferHelper;
		bufferHelper.CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(), 
			indices, m_IndexBuffer.get());
	}
}