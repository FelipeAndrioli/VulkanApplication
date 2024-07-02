#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <optional>
#include <string>
#include <set>
#include <algorithm>
#include <assert.h>

#include "VulkanHeader.h"
#include "Window.h"
#include "CommandBuffer.h"
#include "Graphics.h"
#include "DeviceMemory.h"

namespace Engine::Graphics {
	const int FRAMES_IN_FLIGHT = 2;
	const int DEDICATED_GPU = 2;
	const std::vector<const char*> c_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t>graphicsFamily;
		std::optional<uint32_t>presentFamily;
		std::optional<uint32_t>graphicsAndComputeFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value() && graphicsAndComputeFamily.has_value();
		}
	};

	VkPhysicalDevice CreatePhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);
	bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	VkSampleCountFlagBits GetMaxSampleCount(VkPhysicalDeviceProperties deviceProperties);

	std::vector<VkPhysicalDevice> m_AvailablePhysicalDevices;

#ifndef NDEBUG
	const bool c_EnableValidationLayers = true;
#else
	const bool c_EnableValidationLayers = false;
#endif

	const std::vector<const char*> c_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	void CreateInstance(VkInstance& instance);
	bool checkValidationLayerSupport();

	void CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface);

	void CreateLogicalDevice(QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice);
	void CreateQueue(VkDevice& logicalDevice, uint32_t queueFamilyIndex, VkQueue& queue);
	void WaitIdle(VkDevice& logicalDevice);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void CreateDebugMessenger(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger);
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct SwapChain {
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;

		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;

		VkFormat swapChainImageFormat;

		VkExtent2D swapChainExtent;

		uint32_t imageIndex;
	};

	void CreateSwapChain(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
	void CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
	void CreateSwapChainImageViews(VkDevice& logicalDevice, SwapChain& swapChain);
	void DestroySwapChain(VkDevice& logicalDevice, SwapChain& swapChain);
	void RecreateSwapChain(Window& window, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain);
	void CreateSwapChainSemaphores(VkDevice& logicalDevice, SwapChain& swapChain);
	void CreateCommandPool(VkDevice& logicalDevice, VkCommandPool& commandPool, uint32_t queueFamilyIndex);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>&availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, const VkExtent2D & extent);
	void BeginCommandBuffer(VkDevice& logicalDevice, VkCommandPool& commandPool, VkCommandBuffer& commandBuffer);
	void EndCommandBuffer(VkCommandBuffer& commandBuffer);

	VkCommandBuffer BeginSingleTimeCommandBuffer(VkDevice& logicalDevice, VkCommandPool& commandPool);
	void EndSingleTimeCommandBuffer(VkDevice& logicalDevice, VkQueue& queue, VkCommandBuffer& commandBuffer, VkCommandPool& commandPool);
	
	class GraphicsDevice {
	public:
		GraphicsDevice(Window& window);
		~GraphicsDevice();

		bool CreateSwapChain(Window& window, SwapChain& swapChain);

		void WaitIdle();
		void CreateFramesResources();
		void BindViewport(const Viewport& viewport, VkCommandBuffer& commandBuffer);
		void BindScissor(const Rect& rect, VkCommandBuffer& commandBuffer);
		void BeginRenderPass(const VkRenderPass& renderPass, VkCommandBuffer& commandBuffer, const VkFramebuffer& framebuffer, const VkExtent2D renderArea);
		void EndRenderPass(VkCommandBuffer& commandBuffer);

		VkCommandBuffer* BeginFrame(SwapChain& swapChain);
		void EndFrame(const VkDevice& logicalDevice, const VkCommandBuffer& commandBuffer, const SwapChain& swapChain);
		void PresentFrame(const SwapChain& swapChain);

		GraphicsDevice& CreateImage(GPUImage& image, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType);
		GraphicsDevice& CreateImageView(GPUImage& image, const VkImageViewType viewType, const VkImageAspectFlags aspectFlags, const uint32_t layerCount);
		GraphicsDevice& RecreateImageView(GPUImage& image);
		GraphicsDevice& AllocateMemory(GPUImage& image, VkMemoryPropertyFlagBits memoryProperty);

		GraphicsDevice& TransitionImageLayout(GPUImage& image, VkImageLayout newLayout);
		GraphicsDevice& GenerateMipMaps(GPUImage& image);
		GraphicsDevice& CreateImageSampler(GPUImage& image, VkSamplerAddressMode addressMode);
		GraphicsDevice& RecreateImage(GPUImage& image);
		GraphicsDevice& ResizeImage(GPUImage& image, uint32_t width, uint32_t height);
		GraphicsDevice& CopyBufferToImage(GPUImage& image, VkBuffer& srcBuffer);

		template <class T>
		GraphicsDevice& UploadDataToImage(GPUImage& image, const T* data, const size_t dataSize);

		GraphicsDevice& DestroyImage(GPUImage& image);

		void CreateFramebuffer(const VkRenderPass& renderPass, std::vector<VkImageView&> attachmentViews, VkExtent2D& framebufferExtent);
		void CreateDepthBuffer(GPUImage& depthBuffer, uint32_t width, uint32_t height);
		void CreateRenderTarget(GPUImage& renderTarget, uint32_t width, uint32_t height, VkFormat imageFormat);
		GraphicsDevice& CreateTexture2D(GPUImage& texture, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat imageFormat);
		GraphicsDevice& CreateCubeTexture(GPUImage& texture, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat imageFormat);

		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;

		QueueFamilyIndices m_QueueFamilyIndices;
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		VkQueue m_ComputeQueue;

		VkFence frameFences[FRAMES_IN_FLIGHT];

		uint32_t currentFrame = 0;

		VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];

		VkFramebuffer framebuffer;
	};
}