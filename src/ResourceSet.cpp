#include "ResourceSet.h"

namespace Engine {
	ResourceSet::ResourceSet(
		ResourceSetLayout& resourceSetLayout, 
		LogicalDevice& logicalDevice, 
		PhysicalDevice& physicalDevice, 
		CommandPool& commandPool, 
		const SwapChain& swapChain, 
		const DepthBuffer& depthBuffer, 
		const VkRenderPass& renderPass,
		std::vector<Assets::Model*>& models
	):
		ResourceSetIndex(resourceSetLayout.ResourceSetIndex) {

		m_GraphicsPipeline.reset(new class GraphicsPipeline(resourceSetLayout, logicalDevice, swapChain, depthBuffer, renderPass));
		
		if (resourceSetLayout.RenderType == ResourceSetLayout::RenderType::DEFAULT_RENDER) {
			SetModelResources(physicalDevice, logicalDevice, models);
			CreateVertexBuffer(physicalDevice, logicalDevice, commandPool);
			CreateIndexBuffer(physicalDevice, logicalDevice, commandPool);
		}
	}
	
	ResourceSet::~ResourceSet() {
		m_Vertices.clear();
		m_Indices.clear();

		m_GraphicsPipeline.reset();
		m_VertexBuffer.reset();
		m_IndexBuffer.reset();
	}

	void ResourceSet::SetModelResources(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice, 
		std::vector<Assets::Model*>& models) {
		m_Vertices.clear();
		m_Indices.clear();

		for (auto model : models) {

			if (model->ResourceSetIndex != ResourceSetIndex) continue;

			VkDeviceSize bufferSize = sizeof(Engine::UniformBufferObject);
			model->m_UniformBuffer.reset(new class Engine::Buffer(Engine::MAX_FRAMES_IN_FLIGHT, logicalDevice, physicalDevice,
				bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
			model->m_UniformBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			model->m_UniformBuffer->BufferMemory->MapMemory();

			model->m_DescriptorSets.reset(new class Engine::DescriptorSets(logicalDevice.GetHandle(), 
				m_GraphicsPipeline->GetDescriptorPool().GetHandle(), m_GraphicsPipeline->GetDescriptorSetLayout().GetHandle(), 
				model->m_UniformBuffer.get())
			);

			m_Vertices.insert(m_Vertices.end(), model->GetVertices().begin(), model->GetVertices().end());
			m_Indices.insert(m_Indices.end(), model->GetIndices().begin(), model->GetIndices().end());
		}
	}

	void ResourceSet::CreateVertexBuffer(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice, CommandPool& commandPool) {

		if (m_Vertices.size() == 0) return;

		VkDeviceSize bufferSize = sizeof(Assets::Vertex) * m_Vertices.size();
		
		m_VertexBuffer.reset(new class Buffer(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT), logicalDevice, physicalDevice, bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
		m_VertexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferHelper bufferHelper;
		bufferHelper.CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(),
			m_Vertices, m_VertexBuffer.get());
	}

	void ResourceSet::CreateIndexBuffer(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice, CommandPool& commandPool) {

		if (m_Indices.size() == 0) return;
		VkDeviceSize bufferSize = sizeof(uint16_t) * m_Indices.size();

		m_IndexBuffer.reset(new class Buffer(1, logicalDevice, physicalDevice,
			bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
		m_IndexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferHelper bufferHelper;
		bufferHelper.CopyFromStaging(logicalDevice, physicalDevice, commandPool.GetHandle(), 
			m_Indices, m_IndexBuffer.get());
	}

}