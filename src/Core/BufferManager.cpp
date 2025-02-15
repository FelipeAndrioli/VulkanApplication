#include "BufferManager.h"

#include "GraphicsDevice.h"

Graphics::BufferManager::BufferManager() {

}

Graphics::BufferManager::~BufferManager() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->DestroyBuffer(m_MainBuffer);
}

void Graphics::BufferManager::CreateMainBuffer() {

	Graphics::BufferDescription desc = {};
	desc.Capacity = m_Capacity;
	desc.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	desc.MemoryProperty = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->CreateBuffer(desc, m_MainBuffer, desc.Capacity);

	std::cout << "Main buffer capacity: " << m_Capacity << '\n';
}

Graphics::Buffer Graphics::BufferManager::SubAllocateBuffer(size_t size) {
	if (m_MainBuffer.Handle == VK_NULL_HANDLE)
		CreateMainBuffer();

	assert(m_Size + size < m_Capacity && "Buffer size exceeded!");

	Graphics::Buffer buffer = {};
	buffer.Offset = m_Size;
	buffer.Size = 0;
	buffer.Capacity = size;
	buffer.Handle = &m_MainBuffer.Handle;

	m_Size += size;

	std::cout << "Main buffer suballocation: " << m_Size << '\n';

	return buffer;
}

void Graphics::BufferManager::WriteBuffer(Graphics::Buffer& buffer, void* data, size_t dataSize) {

	if (buffer.Size + dataSize > buffer.Capacity) {
		// TODO: better handle with realocation 
		std::cout << "Buffer out of capacity!" << '\n';
		return;
	}

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->WriteBuffer(m_MainBuffer, data, dataSize, buffer.Offset + buffer.Size);
	
	buffer.Size += dataSize;
}

void Graphics::BufferManager::UpdateBuffer(const Graphics::Buffer& buffer, void* data) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->UpdateBuffer(m_MainBuffer, buffer.Offset, data, buffer.Capacity);
}
