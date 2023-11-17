#include "ResourceSet.h"

namespace Engine {
	ResourceSet::ResourceSet(ResourceSetLayout* resourceSetLayout, LogicalDevice* logicalDevice, 
		PhysicalDevice* physicalDevice, CommandPool* commandPool, SwapChain* swapChain, std::vector<Assets::Model*>& models) 
		: p_LogicalDevice(logicalDevice), p_PhysicalDevice(physicalDevice), p_CommandPool(commandPool), 
		p_SwapChain(swapChain), p_ResourceSetLayout(resourceSetLayout), 
		ResourceSetIndex(resourceSetLayout->ResourceSetIndex) {

		for (auto model : models) {
			if (model->ResourceSetIndex == ResourceSetIndex) p_ResourceSetLayout->MaxDescriptorSets++;
		}

		m_GraphicsPipeline.reset(new class GraphicsPipeline(p_ResourceSetLayout, p_LogicalDevice,
			p_SwapChain));
		CreateFrameBuffers();

		if (p_ResourceSetLayout->MaxDescriptorSets == 0) return;
		
		if (p_ResourceSetLayout->RenderType == ResourceSetLayout::RenderType::DEFAULT_RENDER) {
			SetModelResources(models);
			CreateVertexBuffer();
			CreateIndexBuffer();
		}
	}
	
	ResourceSet::~ResourceSet() {
		CleanUp();

		m_Vertices.clear();
		m_Indices.clear();

		m_GraphicsPipeline.reset();
		m_VertexBuffer.reset();
		m_IndexBuffer.reset();
	}

	void ResourceSet::SetModelResources(std::vector<Assets::Model*>& models) {
		m_Vertices.clear();
		m_Indices.clear();

		for (auto model : models) {

			if (model->ResourceSetIndex != ResourceSetIndex) continue;

			VkDeviceSize bufferSize = sizeof(Engine::UniformBufferObject);
			model->m_UniformBuffer.reset(new class Engine::Buffer(Engine::MAX_FRAMES_IN_FLIGHT, p_LogicalDevice, p_PhysicalDevice,
				bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
			model->m_UniformBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			model->m_UniformBuffer->MapMemory();

			model->m_DescriptorSets.reset(new class Engine::DescriptorSets(p_LogicalDevice->GetHandle(), 
				m_GraphicsPipeline->GetDescriptorPool().GetHandle(), m_GraphicsPipeline->GetDescriptorSetLayout().GetHandle(), 
				model->m_UniformBuffer.get())
			);

			m_Vertices.insert(m_Vertices.end(), model->GetVertices().begin(), model->GetVertices().end());
			m_Indices.insert(m_Indices.end(), model->GetIndices().begin(), model->GetIndices().end());
		}
	}

	void ResourceSet::CreateVertexBuffer() {
	
		VkDeviceSize bufferSize = sizeof(Assets::Vertex) * m_Vertices.size();
		
		m_VertexBuffer.reset(new class Buffer(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT), p_LogicalDevice, p_PhysicalDevice, bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
		m_VertexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferHelper bufferHelper;
		bufferHelper.CopyFromStaging(p_LogicalDevice, p_PhysicalDevice, p_CommandPool->GetHandle(),
			m_Vertices, m_VertexBuffer.get());
	}

	void ResourceSet::CreateIndexBuffer() {

		VkDeviceSize bufferSize = sizeof(uint16_t) * m_Indices.size();

		m_IndexBuffer.reset(new class Buffer(1, p_LogicalDevice, p_PhysicalDevice,
			bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
		m_IndexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferHelper bufferHelper;
		bufferHelper.CopyFromStaging(p_LogicalDevice, p_PhysicalDevice, p_CommandPool->GetHandle(), 
			m_Indices, m_IndexBuffer.get());

	}

	void ResourceSet::CreateFrameBuffers() {
		m_Framebuffers.resize(p_SwapChain->GetSwapChainImageViews().size());

		for (size_t i = 0; i < p_SwapChain->GetSwapChainImageViews().size(); i++) {
			VkImageView attachments[] = { p_SwapChain->GetSwapChainImageViews()[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_GraphicsPipeline->GetRenderPass().GetHandle();
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = p_SwapChain->GetSwapChainExtent().width;
			framebufferInfo.height = p_SwapChain->GetSwapChainExtent().height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(p_LogicalDevice->GetHandle(), &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void ResourceSet::CleanUp() {
		for (size_t i = 0; i < m_Framebuffers.size(); i++) {
			vkDestroyFramebuffer(p_LogicalDevice->GetHandle(), m_Framebuffers[i], nullptr);
		}
	}

	void ResourceSet::Resize() {
		CleanUp();
		CreateFrameBuffers();
	}
}