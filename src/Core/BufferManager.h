#pragma once

#include <cassert>

#include "VulkanHeader.h"
#include "Graphics.h"

namespace Graphics {
	extern GPUImage g_MsaaSceneColor;
	extern GPUImage g_SceneDepth;
	extern GPUImage g_SceneColor;
	extern GPUImage g_ResolvedDepth;
	extern GPUImage g_PostEffects;

	void InitializeRenderingImages(uint32_t width, uint32_t height);
	void ResizeDisplayDependentImages(uint32_t width, uint32_t height);
	void ShutdownRenderingImages();

	class BufferManager {
	public:
		BufferManager();
		~BufferManager();

		Graphics::Buffer SubAllocateBuffer(size_t size);

		void WriteBuffer(const Graphics::Buffer& buffer, void* data);
		void UpdateBuffer(const Graphics::Buffer& buffer, void* data);

		// TODO: delete - when a buffer is not longer needed and we can get rid of its sub allocation
		// TODO: resize - when a bigger/smaller buffer is needed and we need to resize the size of existing buffer
	private:
		void CreateMainBuffer();
	private:
		Graphics::GPUBuffer m_MainBuffer;

		size_t m_Size = 0;
		size_t m_Capacity = VK_WHOLE_SIZE;
	};
}