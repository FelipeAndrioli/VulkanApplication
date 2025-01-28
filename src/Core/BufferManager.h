#pragma once

#include <cassert>

#include "VulkanHeader.h"
#include "Graphics.h"

namespace Graphics {
	class BufferManager {
	public:
		BufferManager();
		~BufferManager();

		Graphics::Buffer SubAllocateBuffer(size_t size);

		void WriteBuffer(Graphics::Buffer& buffer, void* data, size_t dataSize);
		void UpdateBuffer(const Graphics::Buffer& buffer, void* data);

		// TODO: delete - when a buffer is not longer needed and we can get rid of its sub allocation
		// TODO: resize - when a bigger/smaller buffer is needed and we need to resize the size of existing buffer
	private:
		void CreateMainBuffer();
	private:
		Graphics::GPUBuffer m_MainBuffer;

		size_t m_Size = 0;
		size_t m_Capacity = 256 * 100000;
	};
}