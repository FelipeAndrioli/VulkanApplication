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

#include "../Assets/Mesh.h"

namespace Graphics {
	class IRenderTarget;
	class SwapChainRenderTarget;

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

	typedef enum RenderPassFlags : uint32_t {
		// attachments
		eColorAttachment			= 0x00000001,
		eDepthAttachment			= 0x00000002,
		eResolveAttachment			= 0x00000004,

		// op operations
		eColorLoadOpClear			= 0x00000008,
		eColorLoadOpLoad			= 0x00000010,
		eColorStoreOpStore			= 0x00000020,

		// layouts
		eInitialLayoutColorOptimal	= 0x00000040,
		eFinalLayoutTransferSrc		= 0x00000080,
		eFinalLayoutTransferDst		= 0x00000100,
		eFinalLayoutPresent			= 0x00000200
	} RenderPassFlags;

	struct RenderPassDesc {
		VkOffset2D					Offset				= {};
		VkViewport					Viewport			= {};
		VkExtent2D					Extent				= {};
		VkRect2D					Scissor				= {};
		VkSampleCountFlagBits		SampleCount			= VK_SAMPLE_COUNT_1_BIT;
		uint32_t					Flags				= 0;
		VkFormat					ColorImageFormat	= {};
		VkFormat					DepthImageFormat	= {};

		std::vector<VkClearValue>	ClearValues;
	};

	struct RenderPass {
		RenderPassDesc Description = {};

		VkRenderPass Handle = VK_NULL_HANDLE;

		std::vector<VkAttachmentDescription> Attachments = {};
		std::vector<VkSubpassDependency> Dependencies = {};
		std::vector<VkSubpassDescription> Subpasses = {};

		VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct SwapChain {
		VkSwapchainKHR Handle = VK_NULL_HANDLE;
		VkExtent2D Extent = { 0, 0 };
		uint32_t ImageIndex = 0;

		std::vector<VkImage> Images;
		std::vector<VkImageView> ImageViews;
		std::vector<VkSampler> ImageSamplers;

		std::unique_ptr<SwapChainRenderTarget> RenderTarget;
		VkFormat ImageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	};

	struct Shader {
		std::string filename;
		VkShaderStageFlagBits stage;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo shaderStageInfo;

		std::vector<char> sourceCode;
		std::vector<unsigned int> spirv;
	};

	struct InputLayout {
		std::vector<VkPushConstantRange>			pushConstants;
		std::vector<VkDescriptorSetLayoutBinding>	bindings;
	};

	struct PipelineStateDescription {
		const Shader* vertexShader		= nullptr;
		const Shader* fragmentShader	= nullptr;
		const Shader* geometryShader	= nullptr;
		const Shader* computeShader		= nullptr;

		bool noVertex					= false;
		bool depthTestEnable			= true;
		bool depthWriteEnable			= true;
		bool stencilTestEnable			= false;
		bool colorBlendingEnable		= false;

		VkStencilOpState stencilState	= {};

		VkPrimitiveTopology topology	= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode polygonMode		= VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode		= VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace			= VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkExtent2D pipelineExtent		= {};

		float lineWidth					= 1.0f;
		
		VkPipelineColorBlendAttachmentState colorBlendingDesc = {};

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
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		VkPipelineColorBlendStateCreateInfo colorBlending = {};

		const RenderPass* renderPass = nullptr;

		PipelineStateDescription description = {};
	};

	struct Frame {
		VkFence renderFence;

		VkSemaphore renderSemaphore;
		VkSemaphore swapChainSemaphore;

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		VkDescriptorSet bindlessSet;
	};

	class BufferManager;

	class GraphicsDevice {
	public:
		GraphicsDevice(Window& window);
		~GraphicsDevice();

		bool CreateSwapChain(Window& window, SwapChain& swapChain);
		void CreateSwapChainRenderTarget();
		void RecreateSwapChain(Window& window);
		void DestroySwapChain(SwapChain& swapChain);

		void WaitIdle();
		void CreateFrameResources(Frame& frame);
		void DestroyFrameResources(Frame& frame);
		void BindViewport(const Viewport& viewport, VkCommandBuffer& commandBuffer);
		void BindScissor(const Rect& rect, VkCommandBuffer& commandBuffer);
		void BeginRenderPass(const RenderPass& renderPass, const VkCommandBuffer& commandBuffer);
		void EndRenderPass(const VkCommandBuffer& commandBuffer);
		
		void BeginCommandBuffer(VkCommandBuffer& commandBuffer);
		void EndCommandBuffer(VkCommandBuffer& commandBuffer);

		bool BeginFrame(Frame& frame);
		void EndFrame(const Frame& frame);
		void PresentFrame(const Frame& frame);

		VkCommandBuffer BeginSingleTimeCommandBuffer(VkCommandPool& commandPool);
		void EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer, VkCommandPool& commandPool);

		void CreateImage(GPUImage& image, const ImageDescription& description);
		void CreateImageView(const VkImage& image, VkImageView& imageView, const ImageDescription& description);
		void CreateImageView(GPUImage& image);
		void RecreateImageView(GPUImage& image);
		
		void AllocateMemory(GPUImage& image, VkMemoryPropertyFlagBits memoryProperty);
		void AllocateMemory(GPUBuffer& buffer, VkMemoryPropertyFlagBits memoryProperty);

		void TransitionImageLayout(const VkImage& image, const VkImageLayout oldLayout, const VkImageLayout newLayout, const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask, const VkPipelineStageFlags srcPipelineStage, const VkPipelineStageFlags dstPipelineStage);
		void TransitionImageLayout(GPUImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void TransitionImageLayout(GPUImage& image, VkImageLayout newLayout);
		void TransitionImageLayout(const VkImage& image, const VkImageLayout oldLayout, const VkImageLayout newLayout, const VkImageSubresourceRange subresourceRange, const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask, const VkPipelineStageFlags srcPipelineStage, const VkPipelineStageFlags dstPipelineStage);
		void TransitionCubeImageLayout(GPUImage& cubeImage, VkImageLayout newLayout);
		void GenerateMipMaps(GPUImage& image);
		void CreateImageSampler(GPUImage& image);
		void ResizeImage(GPUImage& image, uint32_t width, uint32_t height);
		void CopyBufferToImage(GPUImage& image, GPUBuffer& srcBuffer);
		void DestroyImage(GPUImage& image);
		void DestroyImageCube(GPUImageCube& image);

		template <class T>
		void UploadDataToImage(GPUImage& dstImage, const T* data, const size_t dataSize);

		void CreateFramebuffer(const VkRenderPass& renderPass, const std::vector<VkImageView>& attachmentViews, const VkExtent2D extent, VkFramebuffer& framebuffer, const uint32_t layers = 1);
		void CreateDepthBuffer(GPUImage& depthBuffer, const RenderPassDesc& renderPassDesc);
		void CreateDepthBuffer(GPUImage& depthBuffer, const VkExtent2D& extent, const VkSampleCountFlagBits& samples);
		void CreateDepthOnlyBuffer(GPUImage& depthBuffer, const VkExtent2D extent, const VkSampleCountFlagBits sampleCount, const uint32_t layers);
		void CreateRenderTarget(GPUImage& renderTarget, const RenderPassDesc& renderPassDesc, VkFormat format);
		void CreateRenderTarget(GPUImage& renderTarget, const VkFormat& format, const VkExtent2D& extent, const VkSampleCountFlagBits& samples);

		template <class T>
		void CopyDataFromStaging(GPUBuffer& dstBuffer, T* data, size_t dataSize, size_t offset);

		void CreateBuffer(BufferDescription& desc, GPUBuffer& buffer, size_t bufferSize);
		Buffer CreateBuffer(size_t size);
		GPUBuffer CreateStorageBuffer(size_t size);
		void UpdateBuffer(GPUBuffer& buffer, VkDeviceSize offset, void* data, size_t dataSize);
		void UpdateBuffer(Buffer& buffer, void* data);
		void WriteBuffer(GPUBuffer& buffer, const void* data, size_t size = 0, size_t offset = 0);
		void WriteSubBuffer(Buffer& buffer, void* data, size_t dataSize);

		void CreateTexture(ImageDescription& desc, Texture& texture, Texture::TextureType textureType, void* initialData, size_t dataSize);

		void CopyBuffer(GPUBuffer& srcBuffer, GPUBuffer& dstBuffer, VkDeviceSize size, size_t srcOffset, size_t dstOffset);
		void DestroyBuffer(GPUBuffer& buffer);
	
		void CreateRenderPass(RenderPass& renderPass);
		void DestroyRenderPass(VkRenderPass& renderPass);
		void DestroyFramebuffer(std::vector<VkFramebuffer>& framebuffers);

		void CreateUI(Window& window, VkRenderPass& renderPass);
		void BeginUIFrame();
		void EndUIFrame(const VkCommandBuffer& commandBuffer);
	
		void CreateDescriptorPool(const VkDescriptorPool& descriptorPool, const VkDescriptorPoolSize& poolSizes);
		void CreateDescriptorPool();
		void DestroyDescriptorPool();
		void DestroyDescriptorPool(VkDescriptorPool& descriptorPool);
		void CreatePipelineLayout(PipelineLayoutDesc desc, VkPipelineLayout& pipelineLayout);
		void CreatePipelineLayout(const VkDescriptorSetLayout& descriptorSetLayout, VkPipelineLayout& pipelineLayout, const std::vector<VkPushConstantRange>& pushConstants);

		void CreateDescriptorSetLayout(VkDescriptorSetLayout& layout, const std::vector<VkDescriptorSetLayoutBinding> bindings);
		void CreateDescriptorSetLayout(VkDescriptorSetLayout& layout, const VkDescriptorSetLayoutCreateInfo& layoutInfo);
		void DestroyDescriptorSetLayout(VkDescriptorSetLayout& layout);
		void CreateDescriptorSet(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, VkDescriptorSet& descriptorSet);
		void CreateDescriptorSet(VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSet& descriptorSet);
		void CreateDescriptorSet(VkDescriptorPool& descriptorPool, VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSet& descriptorSet);
		void BindDescriptorSet(VkDescriptorSet& descriptorSet, const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout, uint32_t set, uint32_t setCount);

		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const GPUBuffer& buffer);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const Buffer& buffer);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, std::vector<Texture>& textures);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, Texture& texture);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const GPUImage& image);
		void WriteDescriptor(const VkDescriptorSetLayoutBinding binding, const VkDescriptorSet& descriptorSet, const VkImageLayout& imageLayout, const VkImageView& imageView, const VkSampler& imageSampler);

#ifdef RUNTIME_SHADER_COMPILATION
		static EShLanguage FindLanguage(const Shader& shader);
		static bool CompileShader(Shader& shader);
#endif

		std::vector<char> ReadFile(const std::string& filename);
		void LoadShader(VkShaderStageFlagBits shaderStage, Shader& shader, const std::string filename);
		void DestroyShader(Shader& shader);
		void CreatePipelineState(PipelineStateDescription& desc, PipelineState& pso, const IRenderTarget& renderTarget);
		void DestroyPipelineLayout(VkPipelineLayout& pipelineLayout);
		void DestroyPipeline(PipelineState& pso);

		uint32_t GetCurrentFrameIndex() { return m_CurrentFrame; }

		VkExtent2D& GetSwapChainExtent() { return m_SwapChain.Extent; }
		const SwapChain& GetSwapChain() { return m_SwapChain; }

		Frame& GetCurrentFrame();
		Frame& GetLastFrame();
		Frame& GetFrame(int i);

		void DestroyRenderPass(RenderPass& renderPass);
		void ResizeRenderPass(const uint32_t width, const uint32_t height, RenderPass& renderPass);

		VkFormat GetDepthFormat()		{ return FindDepthFormat(m_PhysicalDevice); }
		VkFormat GetDepthOnlyFormat()	{ return VK_FORMAT_D32_SFLOAT; }

	public:
		VkDevice					m_LogicalDevice		= VK_NULL_HANDLE;
		VkPhysicalDevice			m_PhysicalDevice	= VK_NULL_HANDLE;
		VkSurfaceKHR				m_Surface			= VK_NULL_HANDLE;
		VkInstance					m_VulkanInstance	= VK_NULL_HANDLE;
		VkCommandPool				m_CommandPool		= VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT	m_DebugMessenger	= VK_NULL_HANDLE;

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
	
		Graphics::SwapChain m_SwapChain;
	private:
		VkPhysicalDevice CreatePhysicalDevice(VkInstance& instance, VkSurfaceKHR& surface);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		bool isDeviceSuitable(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		void CreateSurface(VkInstance& instance, GLFWwindow& window, VkSurfaceKHR& surface);
		void CreateInstance(VkInstance& instance);
		void CreateLogicalDevice(QueueFamilyIndices indices, VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice);
		void CreateQueue(VkDevice& logicalDevice, uint32_t queueFamilyIndex, VkQueue& queue);
		void CreateSwapChainImageViews(SwapChain& swapChain);
		
		void CreateCommandPool(VkCommandPool& commandPool, uint32_t queueFamilyIndex);
		void CreateCommandBuffer(VkCommandPool& commandPool, VkCommandBuffer& commandBuffer);
		

		SwapChainSupportDetails QuerySwapChainSupportDetails(VkPhysicalDevice& device, VkSurfaceKHR& surface);
		std::vector<const char*> GetRequiredExtensions();
		bool CheckValidationLayerSupport();
		void CheckRequiredExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions, std::vector<VkExtensionProperties> vulkanSupportedExtensions);
		VkFormat FindSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		bool HasStencilComponent(VkFormat format);
		VkFormat FindDepthFormat(VkPhysicalDevice& physicalDevice);
		VkFormat FindDepthOnlyFormat();
		void CreateSwapChainInternal(VkPhysicalDevice& physicalDevice, VkDevice& logicalDevice, VkSurfaceKHR& surface, SwapChain& swapChain, VkExtent2D currentExtent);
		void CreateImage(GPUImage& image);
	};

	inline GraphicsDevice*& GetDevice() {
		static GraphicsDevice* device = nullptr;
		return device;
	}
}
