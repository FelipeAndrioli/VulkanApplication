#include "BufferManager.h"

#include "GraphicsDevice.h"

namespace Graphics {
	GPUImage g_SceneColor;
	GPUImage g_SceneDepth;
	GPUImage g_PostEffects;
	GPUImage g_ResolvedColor;
	GPUImage g_ResolvedDepth;
};

void Graphics::InitializeRenderingImages(uint32_t width, uint32_t height) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	// old format VK_FORMAT_R8G8B8A8_SRGB,
	gfxDevice->CreateRenderTarget(g_SceneColor, gfxDevice->GetSwapChain().swapChainImageFormat, { width, height }, gfxDevice->m_MsaaSamples);
	gfxDevice->CreateImageSampler(g_SceneColor);
	
	gfxDevice->CreateDepthBuffer(g_SceneDepth, { width, height }, gfxDevice->m_MsaaSamples);

	// old format VK_FORMAT_R8G8B8A8_SRGB
	gfxDevice->CreateRenderTarget(g_PostEffects, gfxDevice->GetSwapChain().swapChainImageFormat, {width, height}, VK_SAMPLE_COUNT_1_BIT);
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gfxDevice->CreateImageSampler(g_PostEffects);

	gfxDevice->CreateRenderTarget(g_ResolvedColor, gfxDevice->GetSwapChain().swapChainImageFormat, { width, height }, VK_SAMPLE_COUNT_1_BIT);
	gfxDevice->CreateImageSampler(g_ResolvedColor);

	gfxDevice->CreateDepthBuffer(g_ResolvedDepth, { width, height }, VK_SAMPLE_COUNT_1_BIT);
}

void Graphics::ResizeDisplayDependentImages(uint32_t width, uint32_t height) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->ResizeImage(g_SceneColor, width, height);
	gfxDevice->CreateImageSampler(g_SceneColor);
	
	gfxDevice->ResizeImage(g_SceneDepth, width, height);

	gfxDevice->ResizeImage(g_PostEffects, width, height);
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gfxDevice->CreateImageSampler(g_PostEffects);

	gfxDevice->ResizeImage(g_ResolvedColor, width, height);
	gfxDevice->CreateImageSampler(g_ResolvedColor);

	gfxDevice->ResizeImage(g_ResolvedDepth, width, height);
}

void Graphics::ShutdownRenderingImages() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyImage(g_SceneColor);
	gfxDevice->DestroyImage(g_SceneDepth);
	gfxDevice->DestroyImage(g_PostEffects);
	gfxDevice->DestroyImage(g_ResolvedColor);
	gfxDevice->DestroyImage(g_ResolvedDepth);
}

Graphics::BufferManager::BufferManager() {

}

Graphics::BufferManager::~BufferManager() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->DestroyBuffer(m_MainBuffer);
}

void Graphics::BufferManager::CreateMainBuffer() {

	// need to do a better job calculating the capacity, or even make it dynamic 
	m_Capacity = 256 * 100000;

	Graphics::BufferDescription desc = {};
	desc.BufferSize = m_Capacity;
	desc.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	desc.MemoryProperty = static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->CreateBuffer(desc, m_MainBuffer, desc.BufferSize);

	std::cout << "Main buffer capacity: " << m_Capacity << '\n';
}

Graphics::Buffer Graphics::BufferManager::SubAllocateBuffer(size_t size) {
	if (m_MainBuffer.Handle == VK_NULL_HANDLE)
		CreateMainBuffer();

	assert(m_Size + size < m_Capacity && "Buffer size exceeded!");

	Graphics::Buffer buffer = {};
	buffer.Offset = m_Size;
	buffer.Size = size;
	buffer.Handle = &m_MainBuffer.Handle;

	m_Size += size;

	std::cout << "Main buffer suballocation: " << m_Size << '\n';

	return buffer;
}

void Graphics::BufferManager::WriteBuffer(const Graphics::Buffer& buffer, void* data) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->WriteBuffer(m_MainBuffer, data, buffer.Size, buffer.Offset);
}

void Graphics::BufferManager::UpdateBuffer(const Graphics::Buffer& buffer, void* data) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	gfxDevice->UpdateBuffer(m_MainBuffer, buffer.Offset, data, buffer.Size);
}
