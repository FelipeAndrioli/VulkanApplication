#include "BufferManager.h"

#include "GraphicsDevice.h"

BufferManager::BufferManager() {

}

BufferManager::~BufferManager() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->DestroyBuffer(m_MainBuffer);
}

void BufferManager::CreateMainBuffer() {

	// need to do a better job calculating the capacity, or even make it dynamic 
	m_Capacity = 256 * 100000;

	Graphics::BufferDescription desc = {};
	desc.BufferSize = m_Capacity;
	desc.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	desc.MemoryProperty = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->CreateBuffer(desc, m_MainBuffer, desc.BufferSize);
}

Graphics::Buffer BufferManager::SubAllocateBuffer(size_t size) {
	if (m_MainBuffer.Handle == VK_NULL_HANDLE)
		CreateMainBuffer();

	assert(m_Size + size < m_Capacity && "Buffer size exceeded!");

	Graphics::Buffer buffer = {};
	buffer.Offset = m_Size;
	buffer.Size = size;
	buffer.Handle = &m_MainBuffer.Handle;

	m_Size += size;
	
	return buffer;
}

void BufferManager::WriteBuffer(const Graphics::Buffer& buffer, void* data) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->WriteBuffer(m_MainBuffer, data, buffer.Size, buffer.Offset);
}

void BufferManager::UpdateBuffer(const Graphics::Buffer& buffer, void* data) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->UpdateBuffer(m_MainBuffer, buffer.Offset, data, buffer.Size);
}
