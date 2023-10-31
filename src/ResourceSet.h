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

#include "../Assets/Scene.h"

namespace Engine {
	class ResourceSet {
	public:

		ResourceSet(GraphicsPipelineLayout* graphicsPipelineLayout,
			LogicalDevice* logicalDevice, PhysicalDevice* physicalDevice, 
			CommandPool* commandPool, SwapChain* swapChain, std::vector<Assets::Model*>& models);

		~ResourceSet();
		
		void SetModelResources(std::vector<Assets::Model*>& models);
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateFrameBuffers();
		void CleanUp();
		void Resize();

		GraphicsPipeline* GetGraphicsPipeline() { return m_GraphicsPipeline.get(); };
		VkFramebuffer* GetFramebuffer(size_t index) { return &m_Framebuffers[index]; };

		std::vector<VkFramebuffer>& GetFramebuffers() { return m_Framebuffers; };
		
		inline Buffer* GetVertexBuffers() const { return m_VertexBuffer.get(); };
		inline Buffer* GetIndexBuffers() const { return m_IndexBuffer.get(); };

		inline std::vector<Assets::Vertex>& GetVertices() { return m_Vertices; };
		inline std::vector<uint16_t>& GetIndices() { return m_Indices; };

		const std::string& GetID() const { return m_ID; };

	private:
		std::unique_ptr<class GraphicsPipeline> m_GraphicsPipeline;
		std::unique_ptr<class Buffer> m_VertexBuffer;
		std::unique_ptr<class Buffer> m_IndexBuffer;

		std::vector<VkFramebuffer> m_Framebuffers;
		std::vector<Assets::Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;

		std::string m_ID;

		GraphicsPipelineLayout* p_GraphicsPipelineLayout = nullptr;
		ComputePipelineLayout* p_ComputePipelineLayout = nullptr;

		LogicalDevice* p_LogicalDevice = nullptr;
		PhysicalDevice* p_PhysicalDevice = nullptr;
		CommandPool* p_CommandPool = nullptr;
		SwapChain* p_SwapChain = nullptr;

	};
}
