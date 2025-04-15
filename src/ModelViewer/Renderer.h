#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>

#include "../Core/VulkanHeader.h"
#include "../Core/RenderTarget.h"

#include "glm.hpp"

#define MAX_MODELS 10
#define MAX_CAMERAS 10

namespace Assets {
	class Camera;
	class Model;

	struct Mesh;
}

namespace Graphics {
	struct PipelineState;
	struct GPUBuffer;
	struct RenderPass;
}

enum ModelType;

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
		void RenderMeshes(const VkCommandBuffer& commandBuffer, DrawPass pass, Graphics::IRenderTarget& renderTarget, Graphics::PipelineState* pso);
		void SetCurrentPass(DrawPass pass)	{ m_CurrentPass = pass; }
		void SetCurrentDraw(uint32_t draw)	{ m_CurrentDraw = draw; }
		void ResetDraw()					{ SetCurrentDraw(0); SetCurrentPass(tZPass); }
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
		int CameraIdx = 0;
	};

	extern Graphics::PipelineState m_SkyboxPSO;
	extern Graphics::PipelineState m_ColorPSO;
	extern Graphics::PipelineState m_ColorStencilPSO;
	extern Graphics::PipelineState m_OutlinePSO;
	extern Graphics::PipelineState m_WireframePSO;
	extern Graphics::PipelineState m_LightSourcePSO;
	extern Graphics::PipelineState m_TransparentPSO;
	extern Graphics::PipelineState m_TransparentStencilPSO;
	extern Graphics::PipelineState m_RenderDepthPSO;
	extern Graphics::PipelineState m_RenderNormalsPSO;

	std::shared_ptr<Assets::Model> LoadModel(ModelType modelType);
	std::shared_ptr<Assets::Model> LoadModel(const std::string& path);

	void Init();
	void Shutdown();
	void LoadResources(const Graphics::IRenderTarget& renderTarget, const Graphics::GPUImage& shadowMappingImage);
	void OnUIRender();

	void UpdateGlobalDescriptors(const VkCommandBuffer& commandBuffer, const std::array<Assets::Camera, MAX_CAMERAS> cameras, const bool renderNormalMap, float minShadowBias, float maxShadowBias);
	void RenderSkybox(const VkCommandBuffer& commandBuffer);
	void RenderOutline(const VkCommandBuffer& commandBuffer, Assets::Model& model);
	void RenderWireframe(const VkCommandBuffer& commandBuffer, Assets::Model& model);
	void RenderLightSources(const VkCommandBuffer& commandBuffer);
	void RenderCube(const VkCommandBuffer& commandBuffer, const Graphics::PipelineState& PSO);
	void SetCameraIndex(int index);

	const Graphics::PipelineState& GetPSO(uint16_t flags);
}
