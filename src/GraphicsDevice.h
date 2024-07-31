#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <optional>
#include <string>
#include <set>
#include <algorithm>
#include <assert.h>
#include <memory>

#include "VulkanHeader.h"
#include "Window.h"
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

	struct Shader {
		std::string filename;
		VkShaderStageFlagBits stage;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo shaderStageInfo;

		VkVertexInputBindingDescription BindingDescription = Assets::Vertex::getBindingDescription();
		std::array<VkVertexInputAttributeDescription, 3> AttributeDescriptions = Assets::Vertex::getAttributeDescriptions();
	};

	struct InputLayout {
		std::vector<VkPushConstantRange> pushConstants;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	struct PipelineStateDescription {
		const Shader* vertexShader = nullptr;
		const Shader* fragmentShader = nullptr;
		const Shader* computeShader = nullptr;

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
		VkExtent2D pipelineExtent = {};

		float lineWidth = 1.0f;

		std::vector<InputLayout> psoInputLayout;

		std::string Name;
	};

	struct PipelineState {
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

		std::vector<VkDescriptorSetLayout> descriptorSetLayout;
		std::vector<VkPushConstantRange> pushConstants;
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		std::vector<VkImageViewType> imageViewTypes;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		VkPipelineShaderStageCreateInfo shaderStages = {};
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClip = {};
		VkPipelineViewportStateCreateInfo viewportState = {};
		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		VkSampleMask sampleMask = {};
		VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	};

	void CreateSwapChain(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
	void CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
	void CreateSwapChainImageViews(VkDevice& logicalDevice, SwapChain& swapChain);
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
		void RecreateSwapChain(Window& window, SwapChain& swapChain);
		void DestroySwapChain(SwapChain& swapChain);

		void WaitIdle();
		void CreateFramesResources();
		void BindViewport(const Viewport& viewport, VkCommandBuffer& commandBuffer);
		void BindScissor(const Rect& rect, VkCommandBuffer& commandBuffer);
		void BeginDefaultRenderPass(VkCommandBuffer& commandBuffer, const VkExtent2D renderArea);
		void BeginRenderPass(const VkRenderPass& renderPass, VkCommandBuffer& commandBuffer, const VkExtent2D renderArea);
		void EndRenderPass(VkCommandBuffer& commandBuffer);

		VkCommandBuffer BeginSingleTimeCommandBuffer();
		void EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer);

		VkCommandBuffer* BeginFrame(SwapChain& swapChain);
		void EndFrame(const VkCommandBuffer& commandBuffer, const SwapChain& swapChain);
		void PresentFrame(const SwapChain& swapChain);

		GraphicsDevice& CreateImage(GPUImage& image, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType);
		GraphicsDevice& CreateImageView(GPUImage& image, const VkImageViewType viewType, const VkImageAspectFlags aspectFlags, const uint32_t layerCount);
		GraphicsDevice& RecreateImageView(GPUImage& image);
		
		GraphicsDevice& AllocateMemory(GPUImage& image, VkMemoryPropertyFlagBits memoryProperty);
		GraphicsDevice& AllocateMemory(GPUBuffer& buffer, VkMemoryPropertyFlagBits memoryProperty);

		GraphicsDevice& TransitionImageLayout(GPUImage& image, VkImageLayout newLayout);
		GraphicsDevice& GenerateMipMaps(GPUImage& image);
		GraphicsDevice& CreateImageSampler(GPUImage& image);
		GraphicsDevice& RecreateImage(GPUImage& image);
		GraphicsDevice& ResizeImage(GPUImage& image, uint32_t width, uint32_t height);
		GraphicsDevice& CopyBufferToImage(GPUImage& image, GPUBuffer& srcBuffer);
		GraphicsDevice& DestroyImage(GPUImage& image);

		template <class T>
		GraphicsDevice& UploadDataToImage(GPUImage& dstImage, const T* data, const size_t dataSize);

		void CreateFramebuffer(const VkRenderPass& renderPass, std::vector<VkImageView>& attachmentViews, VkExtent2D& framebufferExtent);
		void CreateDepthBuffer(GPUImage& depthBuffer, uint32_t width, uint32_t height);
		void CreateRenderTarget(GPUImage& renderTarget, uint32_t width, uint32_t height, VkFormat format);

		template <class T>
		void CopyDataFromStaging(GPUBuffer& dstBuffer, T* data, size_t dataSize, size_t offset);

		void CreateBuffer(BufferDescription& desc, GPUBuffer& buffer, size_t bufferSize);
		void UpdateBuffer(GPUBuffer& buffer, VkDeviceSize offset, void* data, size_t dataSize);
		void WriteBuffer(GPUBuffer& buffer, const void* data, size_t size = 0, size_t offset = 0);

		void CreateTexture(ImageDescription& desc, Texture& texture, Texture::TextureType textureType, void* initialData, size_t dataSize);
		void DestroyTexture(Texture& texture);

		GraphicsDevice& CopyBuffer(GPUBuffer& srcBuffer, GPUBuffer& dstBuffer, VkDeviceSize size, size_t srcOffset, size_t dstOffset);
		GraphicsDevice& AddBufferChunk(GPUBuffer& buffer, BufferDescription::BufferChunk newChunk);
		void DestroyBuffer(GPUBuffer& buffer);
	
		void CreateDefaultRenderPass(VkRenderPass& renderPass);
		void CreateRenderPass(VkRenderPass& renderPass, std::vector<VkAttachmentDescription> attachments, std::vector<VkSubpassDescription> subpass, std::vector<VkSubpassDependency> dependencies);
		void DestroyRenderPass(VkRenderPass& renderPass);
		void RecreateDefaultRenderPass(VkRenderPass& renderPass, SwapChain& swapChain);
		void DestroyFramebuffer();

		void DestroyCommandBuffer(VkCommandBuffer& commandBuffer);
		void RecreateCommandBuffers();

		void CreateUI(Window& window, VkRenderPass& renderPass);
		void BeginUIFrame();
		void EndUIFrame(const VkCommandBuffer& commandBuffer);
		
		void CreateDescriptorPool();
		void CreatePipelineLayout(PipelineLayoutDesc desc, VkPipelineLayout& pipelineLayout);
		void CreateDescriptorSet(InputLayout& inputLayout, VkDescriptorSet& descriptorSet);
		void CreateDescriptorSet(VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSet& descriptorSet);
		void BindDescriptorSet(VkDescriptorSet& descriptorSet, const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout, uint32_t set, uint32_t setCount);

		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet,
			const VkBuffer& buffer, const size_t bufferSize, const size_t bufferOffset);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, std::vector<Texture>& textures);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, Texture& texture);

		std::vector<char> ReadFile(const std::string& filename);
		void LoadShader(VkShaderStageFlagBits shaderStage, Shader& shader, const std::string filename);
		void CreateRenderPass(VkFormat colorImageFormat, VkFormat depthFormat, VkRenderPass& renderPass);
		void CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso);
		void CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso, VkRenderPass& renderPass);
		void DestroyPipeline(PipelineState& pso);

		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		VkRenderPass defaultRenderPass = VK_NULL_HANDLE;

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

		VkDescriptorPool descriptorPool;
		uint32_t poolSize = 256;

	private:
		bool isDeviceSuitable(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		std::vector<const char*> GetRequiredExtensions();
		void CheckRequiredExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions, std::vector<VkExtensionProperties> vulkanSupportedExtensions);
		void CreateInstance(VkInstance& instance);
		VkFormat FindSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat(VkPhysicalDevice& physicalDevice);
		void CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
		VkPhysicalDevice CreatePhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);
		void CreateImage(GPUImage& image);
	};

	inline GraphicsDevice*& GetDevice() {
		static GraphicsDevice* device = nullptr;
		return device;
	}
}