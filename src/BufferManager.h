#pragma once

#include <cassert>

#include "VulkanHeader.h"
#include "Graphics.h"

class BufferManager {
public:
	BufferManager();
	~BufferManager();

	Engine::Graphics::Buffer SubAllocateBuffer(size_t size);

	void WriteBuffer(const Engine::Graphics::Buffer& buffer, void* data);
	void UpdateBuffer(const Engine::Graphics::Buffer& buffer, void* data);

	// TODO: delete - when a buffer is not longer needed and we can get rid of its sub allocation
	// TODO: resize - when a bigger/smaller buffer is needed and we need to resize the size of existing buffer
private:
	void CreateMainBuffer();
private:
	Engine::Graphics::GPUBuffer m_MainBuffer;
	
	size_t m_Size = 0;
	size_t m_Capacity = VK_WHOLE_SIZE;
};