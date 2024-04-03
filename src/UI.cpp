#include "UI.h"

#include "CommandBuffer.h"
#include "Settings.h"
#include "Instance.h"
#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "../Assets/Scene.h"

namespace Engine {

	UI::UI(GLFWwindow* window, Instance* instance, PhysicalDevice* physicalDevice, LogicalDevice* logicalDevice, SwapChain* swapChain,
		RenderPass* renderPass, CommandBuffer* commandBuffer, const int minImageCount) : 
		p_Window(window), 
		p_Instance(instance), 
		p_PhysicalDevice(physicalDevice), 
		p_LogicalDevice(logicalDevice), 
		p_SwapChain(swapChain), 
		m_MinImageCount(minImageCount), 
		p_RenderPass(renderPass),
		p_CommandBuffer(commandBuffer) {

		m_UIDescriptorPool = VK_NULL_HANDLE;
		//m_UIRenderPass = VK_NULL_HANDLE;
		m_UICommandPool = VK_NULL_HANDLE;

		Init();
	}

	void UI::Init() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		createUIDescriptorPool(p_LogicalDevice->GetHandle());
		createUICommandPool(p_LogicalDevice->GetHandle(), 
			p_PhysicalDevice->GetQueueFamilyIndices().graphicsFamily.value());
		/*
		createUIRenderPass(p_LogicalDevice->GetHandle(), p_SwapChain->GetSwapChainImageFormat());
		createUICommandBuffers(p_LogicalDevice->GetHandle());
		createUIFrameBuffers(p_LogicalDevice->GetHandle(), p_SwapChain->GetSwapChainExtent(), 
			p_SwapChain->GetSwapChainImageViews());
		*/

		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;

		ImGui_ImplGlfw_InitForVulkan(p_Window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = p_Instance->GetHandle();
		init_info.PhysicalDevice = p_PhysicalDevice->GetHandle();;
		init_info.Device = p_LogicalDevice->GetHandle();
		init_info.QueueFamily = p_PhysicalDevice->GetQueueFamilyIndices().graphicsFamily.value();
		init_info.Queue = p_LogicalDevice->GetGraphicsQueue();
		//init_info.PipelineCache = g_PipelineCache;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_UIDescriptorPool;
		init_info.MinImageCount = m_MinImageCount;
		init_info.ImageCount = m_MinImageCount;
		//init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.MSAASamples = p_PhysicalDevice->GetMsaaSamples();
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, p_RenderPass->GetHandle());

		VkCommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommandBuffer(p_LogicalDevice->GetHandle(), m_UICommandPool);
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		CommandBuffer::EndSingleTimeCommandBuffer(p_LogicalDevice->GetHandle(), p_LogicalDevice->GetGraphicsQueue(), 
			commandBuffer, m_UICommandPool);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void UI::Resize(SwapChain* swapChain) {
		p_SwapChain = swapChain;
		ImGui_ImplVulkan_SetMinImageCount(m_MinImageCount);
	}

	void UI::BeginFrame() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void UI::EndFrame(const VkCommandBuffer& commandBuffer) {
		ImGui::End();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	UI::~UI() {
		std::cout << "Destroying UI" << '\n';

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(p_LogicalDevice->GetHandle(), m_UIDescriptorPool, nullptr);
		vkDestroyCommandPool(p_LogicalDevice->GetHandle(), m_UICommandPool, nullptr);
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

	void UI::createUICommandPool(VkDevice &r_LogicalDevice, uint32_t queueFamilyIndex) {
		
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(r_LogicalDevice, &info, nullptr, &m_UICommandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create UI Command pool!");
		}
	}
}