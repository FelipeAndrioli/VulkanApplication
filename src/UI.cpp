#include "UI.h"

namespace Engine {
	UI::UI() {
		m_UIDescriptorPool = VK_NULL_HANDLE;
		m_UIRenderPass = VK_NULL_HANDLE;
		m_UICommandPool = VK_NULL_HANDLE;
	};

	void UI::Init(GLFWwindow &r_Window, const VkInstance &r_VulkanInstance, const VkPhysicalDevice &r_PhysicalDevice, 
		VkDevice &r_LogicalDevice, const QueueFamilyIndices &r_QueueFamilyIndices, VkQueue &r_GraphicsQueue,
		const VkExtent2D &r_SwapChainExtent, const std::vector<VkImageView> &r_SwapChainImageViews, 
		const VkFormat &r_SwapChainImageFormat, const int minImageCount) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
	
		createUIDescriptorPool(r_LogicalDevice);
		createUIRenderPass(r_LogicalDevice, r_SwapChainImageFormat);
		createUICommandPool(r_LogicalDevice, r_QueueFamilyIndices.graphicsFamily.value());
		createUICommandBuffers(r_LogicalDevice);
		createUIFrameBuffers(r_LogicalDevice, r_SwapChainExtent, r_SwapChainImageViews);

		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;

		ImGui_ImplGlfw_InitForVulkan(&r_Window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = r_VulkanInstance;
		init_info.PhysicalDevice = r_PhysicalDevice;
		init_info.Device = r_LogicalDevice;
		init_info.QueueFamily = r_QueueFamilyIndices.graphicsFamily.value();
		init_info.Queue = r_GraphicsQueue;
		//init_info.PipelineCache = g_PipelineCache;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_UIDescriptorPool;
		init_info.MinImageCount = minImageCount;
		init_info.ImageCount = minImageCount;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, m_UIRenderPass);

		VkCommandBuffer commandBuffer = beginSingleTimeCommands(r_LogicalDevice, m_UICommandPool);
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		endSingleTimeCommands(r_LogicalDevice, r_GraphicsQueue, commandBuffer, m_UICommandPool);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void UI::Resize(VkDevice& r_LogicalDevice, VkExtent2D& r_SwapChainExtent, std::vector<VkImageView>& r_SwapChainImageViews, 
		int minImageCount) {
		ImGui_ImplVulkan_SetMinImageCount(minImageCount);
		createUICommandBuffers(r_LogicalDevice);
		createUIFrameBuffers(r_LogicalDevice, r_SwapChainExtent, r_SwapChainImageViews);
	}

	VkCommandBuffer& UI::GetCommandBuffer(uint32_t currentFrame) {
		return m_UICommandBuffers[currentFrame];
	}

	void UI::Draw(const std::shared_ptr<Layer> &layer) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		layer->OnUIRender();

		ImGui::Render();
	}

	void UI::RecordCommands(const VkExtent2D &r_SwapChainExtent, const uint32_t currentFrame, const uint32_t imageIndex) {
		VkCommandBufferBeginInfo cmdBufferInfo = {};
		cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(m_UICommandBuffers[currentFrame], &cmdBufferInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to start recording UI command buffer!");
		}

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_UIRenderPass;
		renderPassInfo.framebuffer = m_UIFrameBuffers[imageIndex];
		renderPassInfo.renderArea.extent.width = r_SwapChainExtent.width;
		renderPassInfo.renderArea.extent.height = r_SwapChainExtent.height;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_UICommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_UICommandBuffers[currentFrame]);
		vkCmdEndRenderPass(m_UICommandBuffers[currentFrame]);

		if (vkEndCommandBuffer(m_UICommandBuffers[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record UI command buffers!");
		}
	}

	void UI::Destroy(VkDevice& r_LogicalDevice) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		for (auto uiFrameBuffer : m_UIFrameBuffers) {
			vkDestroyFramebuffer(r_LogicalDevice, uiFrameBuffer, nullptr);
		}

		vkFreeCommandBuffers(r_LogicalDevice, m_UICommandPool, 
			static_cast<uint32_t>(m_UICommandBuffers.size()), m_UICommandBuffers.data());
	}

	UI::~UI() {
		std::cout << "Destroying UI" << '\n';
	}

	void UI::createUIDescriptorPool(VkDevice &r_LogicalDevice) {
		VkDescriptorPoolSize pool_sizes[] = {
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;

		if (vkCreateDescriptorPool(r_LogicalDevice, &pool_info, nullptr, &m_UIDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create UI descriptor pool!");
		}
	}

	void UI::createUIRenderPass(VkDevice &r_LogicalDevice, const VkFormat &r_SwapChainImageFormat) {
		VkAttachmentDescription attachDescription = {};
		attachDescription.format = r_SwapChainImageFormat;
		attachDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		VkAttachmentReference attachReference = {};
		attachReference.attachment = 0;
		attachReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachReference;

		// create a subpass dependency to sync main and UI render pass
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &attachDescription;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &subpassDependency;

		if (vkCreateRenderPass(r_LogicalDevice, &renderPassInfo, nullptr, &m_UIRenderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create UI Render pass!");
		}
	}

	void UI::createUICommandPool(VkDevice &r_LogicalDevice, uint32_t queueFamilyIndex) {
		
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(r_LogicalDevice, &info, nullptr, &m_UICommandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create UI Command pool!");
		}
	}

	void UI::createUICommandBuffers(VkDevice &r_LogicalDevice) {
		//m_UICommandBuffers.resize(m_SwapChainImageViews.size());
		m_UICommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_UICommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_UICommandBuffers.size());

		if (vkAllocateCommandBuffers(r_LogicalDevice, &allocInfo, m_UICommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create UI Command buffers!");
		}
	}

	void UI::createUIFrameBuffers(VkDevice &r_LogicalDevice, const VkExtent2D &r_SwapChainExtent, 
		const std::vector<VkImageView> &r_SwapChainImageViews) {
		
		m_UIFrameBuffers.resize(r_SwapChainImageViews.size());

		VkImageView attachment[1];
		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = m_UIRenderPass;
		info.attachmentCount = 1;
		info.pAttachments = attachment;
		info.width = r_SwapChainExtent.width;
		info.height = r_SwapChainExtent.height;
		info.layers = 1;

		for (uint32_t i = 0; i < r_SwapChainImageViews.size(); i++) {
			attachment[0] = r_SwapChainImageViews[i];

			if (vkCreateFramebuffer(r_LogicalDevice, &info, nullptr, &m_UIFrameBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create UI Frame buffers!");
			}
		}
	}
}