#include "BufferManager.h"

#include "GraphicsDevice.h"

namespace Graphics {
	GPUImage g_SceneColor;
	GPUImage g_SceneDepth;
	GPUImage g_PostEffects;
	GPUImage g_FinalDepth;
	GPUImage g_FinalImage;
};

void Graphics::InitializeRenderingImages(uint32_t width, uint32_t height) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->CreateRenderTarget(
		g_SceneColor, 
		VK_FORMAT_R8G8B8A8_SRGB,
		{ width, height }, 
		//gfxDevice->m_MsaaSamples);
		VK_SAMPLE_COUNT_1_BIT);

	gfxDevice->CreateImageSampler(g_SceneColor);

	gfxDevice->CreateRenderTarget(
		g_PostEffects,
		VK_FORMAT_R8G8B8A8_SRGB,
		{ width, height },
		//gfxDevice->m_MsaaSamples);
		VK_SAMPLE_COUNT_1_BIT);
	
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gfxDevice->CreateImageSampler(g_PostEffects);

	gfxDevice->CreateDepthBuffer(g_SceneDepth,
		{ width, height },
		//gfxDevice->m_MsaaSamples);
		VK_SAMPLE_COUNT_1_BIT);

	gfxDevice->CreateDepthBuffer(g_FinalDepth, { width, height }, gfxDevice->m_MsaaSamples);

	gfxDevice->CreateRenderTarget(
		g_FinalImage,
		gfxDevice->GetSwapChain().swapChainImageFormat,
		{ width, height },
		gfxDevice->m_MsaaSamples);

	gfxDevice->TransitionImageLayout(g_FinalImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	gfxDevice->TransitionImageLayout(g_FinalImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gfxDevice->CreateImageSampler(g_FinalImage);
}

void Graphics::ResizeDisplayDependentImages(uint32_t width, uint32_t height) {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->ResizeImage(g_SceneColor, width, height);
	gfxDevice->CreateImageSampler(g_SceneColor);

	gfxDevice->ResizeImage(g_PostEffects, width, height);
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	gfxDevice->TransitionImageLayout(g_PostEffects, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gfxDevice->CreateImageSampler(g_PostEffects);

	gfxDevice->ResizeImage(g_FinalImage, width, height);
	gfxDevice->TransitionImageLayout(g_FinalImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	gfxDevice->TransitionImageLayout(g_FinalImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gfxDevice->CreateImageSampler(g_FinalImage);

	gfxDevice->ResizeImage(g_SceneDepth, width, height);
	gfxDevice->ResizeImage(g_FinalDepth, width, height);
}

void Graphics::ShutdownRenderingImages() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	gfxDevice->DestroyImage(g_SceneColor);
	gfxDevice->DestroyImage(g_SceneDepth);
	gfxDevice->DestroyImage(g_PostEffects);
	gfxDevice->DestroyImage(g_FinalDepth);
	gfxDevice->DestroyImage(g_FinalImage);
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
