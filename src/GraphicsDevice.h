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
#include "BufferManager.h"

#include "Assets/Mesh.h"

namespace Graphics {
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

#ifndef NDEBUG
	const bool c_EnableValidationLayers = true;
#else
	const bool c_EnableValidationLayers = false;
#endif

	const std::vector<const char*> c_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

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
		/*
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		*/

		VkFormat swapChainImageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

		VkExtent2D swapChainExtent = { 0, 0 };

		uint32_t imageIndex = 0;
	};

	struct Shader {
		std::string filename;
		VkShaderStageFlagBits stage;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo shaderStageInfo;

		VkVertexInputBindingDescription BindingDescription = Assets::Vertex::getBindingDescription();
		std::array<VkVertexInputAttributeDescription, 4> AttributeDescriptions = Assets::Vertex::getAttributeDescriptions();
	};

	struct InputLayout {
		std::vector<VkPushConstantRange> pushConstants;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	struct PipelineStateDescription {
		const Shader* vertexShader = nullptr;
		const Shader* fragmentShader = nullptr;
		const Shader* computeShader = nullptr;

		bool noVertex = false;

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

	struct Frame {
		VkFence renderFence;

		VkSemaphore renderSemaphore;
		VkSemaphore swapChainSemaphore;

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		VkDescriptorSet bindlessSet;
	};

	class GraphicsDevice {
	public:
		GraphicsDevice(Window& window);
		~GraphicsDevice();

		bool CreateSwapChain(Window& window, SwapChain& swapChain);
		void RecreateSwapChain(Window& window, SwapChain& swapChain);
		void DestroySwapChain(SwapChain& swapChain);

		void WaitIdle();
		void CreateFrameResources(Frame& frame);
		void DestroyFrameResources(Frame& frame);
		void BindViewport(const Viewport& viewport, VkCommandBuffer& commandBuffer);
		void BindScissor(const Rect& rect, VkCommandBuffer& commandBuffer);
		void BeginRenderPass(const VkRenderPass& renderPass, VkCommandBuffer& commandBuffer, const VkExtent2D renderArea, uint32_t imageIndex, const VkFramebuffer& framebuffer);
		void EndRenderPass(VkCommandBuffer& commandBuffer);
		
		void BeginCommandBuffer(VkCommandBuffer& commandBuffer);
		void EndCommandBuffer(VkCommandBuffer& commandBuffer);

		bool BeginFrame(SwapChain& swapChain, Frame& frame);
		void EndFrame(const SwapChain& swapChain, const Frame& frame);
		void PresentFrame(const SwapChain& swapChain, const Frame& frame);

		VkCommandBuffer BeginSingleTimeCommandBuffer(VkCommandPool& commandPool);
		void EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer, VkCommandPool& commandPool);

		void CreateImage(GPUImage& image, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType);
		void CreateImageView(GPUImage& image);
		void RecreateImageView(GPUImage& image);
		
		void AllocateMemory(GPUImage& image, VkMemoryPropertyFlagBits memoryProperty);
		void AllocateMemory(GPUBuffer& buffer, VkMemoryPropertyFlagBits memoryProperty);

		void TransitionImageLayout(GPUImage& image, VkImageLayout newLayout);
		void GenerateMipMaps(GPUImage& image);
		void CreateImageSampler(GPUImage& image);
		void ResizeImage(GPUImage& image, uint32_t width, uint32_t height);
		void CopyBufferToImage(GPUImage& image, GPUBuffer& srcBuffer);
		void DestroyImage(GPUImage& image);

		template <class T>
		void UploadDataToImage(GPUImage& dstImage, const T* data, const size_t dataSize);

		void CreateFramebuffer(const VkRenderPass& renderPass, const std::vector<VkImageView>& attachmentViews, const VkExtent2D extent, VkFramebuffer& framebuffer);
		void CreateDepthBuffer(GPUImage& depthBuffer, uint32_t width, uint32_t height);
		void CreateRenderTarget(GPUImage& renderTarget, uint32_t width, uint32_t height, VkFormat format);

		template <class T>
		void CopyDataFromStaging(GPUBuffer& dstBuffer, T* data, size_t dataSize, size_t offset);

		void CreateBuffer(BufferDescription& desc, GPUBuffer& buffer, size_t bufferSize);
		Buffer CreateBuffer(size_t size);
		void UpdateBuffer(GPUBuffer& buffer, VkDeviceSize offset, void* data, size_t dataSize);
		void UpdateBuffer(Buffer& buffer, void* data);
		void WriteBuffer(GPUBuffer& buffer, const void* data, size_t size = 0, size_t offset = 0);
		void WriteBuffer(const Buffer& buffer, void* data);

		void CreateTexture(ImageDescription& desc, Texture& texture, Texture::TextureType textureType, void* initialData, size_t dataSize);

		void CopyBuffer(GPUBuffer& srcBuffer, GPUBuffer& dstBuffer, VkDeviceSize size, size_t srcOffset, size_t dstOffset);
		void AddBufferChunk(GPUBuffer& buffer, BufferDescription::BufferChunk newChunk);
		void DestroyBuffer(GPUBuffer& buffer);
	
		void CreateDefaultRenderPass(VkRenderPass& renderPass);
		void CreateRenderPass(VkRenderPass& renderPass, std::vector<VkAttachmentDescription> attachments, std::vector<VkSubpassDescription> subpass, std::vector<VkSubpassDependency> dependencies);
		void DestroyRenderPass(VkRenderPass& renderPass);
		void RecreateDefaultRenderPass(VkRenderPass& renderPass, SwapChain& swapChain);
		void DestroyFramebuffer(std::vector<VkFramebuffer>& framebuffers);

		void CreateUI(Window& window, VkRenderPass& renderPass);
		void BeginUIFrame();
		void EndUIFrame(const VkCommandBuffer& commandBuffer);
	
		void CreateDescriptorPool(const VkDescriptorPool& descriptorPool, const VkDescriptorPoolSize& poolSizes);
		void CreateDescriptorPool();
		void DestroyDescriptorPool();
		void DestroyDescriptorPool(VkDescriptorPool& descriptorPool);
		void CreatePipelineLayout(PipelineLayoutDesc desc, VkPipelineLayout& pipelineLayout);

		void CreateDescriptorSetLayout(VkDescriptorSetLayout& layout, const std::vector<VkDescriptorSetLayoutBinding> bindings);
		void CreateDescriptorSetLayout(VkDescriptorSetLayout& layout, const VkDescriptorSetLayoutCreateInfo& layoutInfo);
		void DestroyDescriptorSetLayout(VkDescriptorSetLayout& layout);
		void CreateDescriptorSet(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, VkDescriptorSet& descriptorSet);
		void CreateDescriptorSet(VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSet& descriptorSet);
		void CreateDescriptorSet(VkDescriptorPool& descriptorPool, VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSet& descriptorSet);
		void BindDescriptorSet(VkDescriptorSet& descriptorSet, const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout, uint32_t set, uint32_t setCount);

		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const Buffer& buffer);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, std::vector<Texture>& textures);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, Texture& texture);

		std::vector<char> ReadFile(const std::string& filename);
		void LoadShader(VkShaderStageFlagBits shaderStage, Shader& shader, const std::string filename);
		void DestroyShader(Shader& shader);
		void CreateRenderPass(VkFormat colorImageFormat, VkFormat depthFormat, VkRenderPass& renderPass);
		void CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso);
		void CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso, VkRenderPass& renderPass);
		void DestroyPipeline(PipelineState& pso);

		void SetSwapChainExtent(VkExtent2D newExtent) { m_SwapChainExtent = newExtent; }
		VkExtent2D& GetSwapChainExtent() { return m_SwapChainExtent; }

		uint32_t GetCurrentFrameIndex() { return m_CurrentFrame; }

		Frame& GetCurrentFrame();
		Frame& GetLastFrame();
		Frame& GetFrame(int i);

		VkRenderPass& GetDefaultRenderPass() { return m_DefaultRenderPass; }
	public:
		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkInstance m_VulkanInstance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		VkRenderPass m_DefaultRenderPass = VK_NULL_HANDLE;

		QueueFamilyIndices m_QueueFamilyIndices;
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		VkQueue m_ComputeQueue;
	private:
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;

		VkDescriptorPool m_DescriptorPool;

		uint32_t m_CurrentFrame = 0;
		uint32_t m_PoolSize = 256;

		VkExtent2D m_SwapChainExtent = { 0, 0 };
		
		Frame m_Frames[FRAMES_IN_FLIGHT] = {};
		
		std::unique_ptr<class BufferManager> m_BufferManager;
	private:
		VkPhysicalDevice CreatePhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		bool isDeviceSuitable(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		void CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface);
		void CreateInstance(VkInstance& instance);
		void CreateLogicalDevice(QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice);
		void CreateQueue(VkDevice& logicalDevice, uint32_t queueFamilyIndex, VkQueue& queue);
		void CreateSwapChainImageViews(VkDevice& logicalDevice, SwapChain& swapChain);
		
		void CreateCommandPool(VkCommandPool& commandPool, uint32_t queueFamilyIndex);
		void CreateCommandBuffer(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer);
		

		SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		std::vector<const char*> GetRequiredExtensions();
		bool CheckValidationLayerSupport();
		void CheckRequiredExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions, std::vector<VkExtensionProperties> vulkanSupportedExtensions);
		VkFormat FindSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat(VkPhysicalDevice& physicalDevice);
		void CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
		void CreateImage(GPUImage& image);
	};

	inline GraphicsDevice*& GetDevice() {
		static GraphicsDevice* device = nullptr;
		return device;
	}
}