#pragma once

#include <cstring>
#include <memory>
#include <string>

#include "./Core/VulkanHeader.h"
#include "glm.hpp"

#define MAX_MODELS 10

namespace Assets {
	class Camera;
	class Model;

	struct Mesh;
}

namespace Graphics {
	struct PipelineState;
	struct GPUBuffer;
}

namespace Renderer {

	class MeshSorter {
	public:

		enum BatchType { tDefault, tShadows };
		enum DrawPass { tZPass, tOpaque, tTransparent, tOutline, tNumPasses };

		struct SortKey {
			uint64_t key;
			uint64_t value;

			DrawPass passId;

			const Graphics::PipelineState* pipelinePtr = nullptr;

			float distance;
		};

		struct SortMesh {
			const Assets::Mesh* mesh;
			Graphics::GPUBuffer* bufferPtr;

			float distance;
			
			uint32_t modelIndex = 0;
			uint32_t totalIndices = 0;
		};

		MeshSorter(BatchType type) {
			m_BatchType = type;
			m_Camera = nullptr;
			m_CurrentPass = tZPass;
			m_CurrentDraw = 0;

			std::memset(m_PassCounts, 0, sizeof(m_PassCounts));
		};

		void SetCamera(const Assets::Camera& camera);
		const Assets::Camera& GetCamera();
		void AddMesh(const Assets::Mesh& mesh, float distance, uint32_t modelIndex, uint32_t totalIndices, Graphics::GPUBuffer& buffer);
		void Sort();
		void RenderMeshes(const VkCommandBuffer& commandBuffer, DrawPass pass);
	private:
		BatchType m_BatchType;
		DrawPass m_CurrentPass;

		uint32_t m_CurrentDraw;
		uint32_t m_PassCounts[tNumPasses];

		const Assets::Camera* m_Camera;

		std::vector<SortKey> m_SortKeys;
		std::vector<SortMesh> m_SortMeshes;
	};

	struct PipelinePushConstants {
		int MaterialIdx = 0;
		int ModelIdx = 0;
		int LightSourceIdx = 0;
	};

	std::shared_ptr<Assets::Model> LoadModel(const std::string& path);

	void Init();
	void Shutdown();
	void LoadResources();
	void OnUIRender();

	void UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const Assets::Camera& camera);
	void RenderSkybox(const VkCommandBuffer& commandBuffer);
	void RenderOutline(const VkCommandBuffer& commandBuffer, Assets::Model& model);
	void RenderWireframe(const VkCommandBuffer& commandBuffer, Assets::Model& model);
	void RenderLightSources(const VkCommandBuffer& commandBuffer);
	void RenderCube(const VkCommandBuffer& commandBuffer, const Graphics::PipelineState& PSO);

	const Graphics::PipelineState& GetPSO(uint16_t flags);
}
