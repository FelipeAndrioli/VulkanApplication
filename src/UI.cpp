#include "UI.h"

#include "GraphicsDevice.h"

namespace Engine {

	UI::UI(GLFWwindow& window) {

		Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

		m_UIDescriptorPool = VK_NULL_HANDLE;
		m_UICommandPool = VK_NULL_HANDLE;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		createUIDescriptorPool(gfxDevice->m_LogicalDevice);
		createUICommandPool(gfxDevice->m_LogicalDevice, gfxDevice->m_QueueFamilyIndices.graphicsFamily.value());

		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;

		ImGui_ImplGlfw_InitForVulkan(&window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = gfxDevice->m_VulkanInstance;
		init_info.PhysicalDevice = gfxDevice->m_PhysicalDevice;
		init_info.Device = gfxDevice->m_LogicalDevice;
		init_info.QueueFamily = gfxDevice->m_QueueFamilyIndices.graphicsFamily.value();
		init_info.Queue = gfxDevice->m_GraphicsQueue;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_UIDescriptorPool;
		init_info.MinImageCount = Graphics::FRAMES_IN_FLIGHT;
		init_info.ImageCount = Graphics::FRAMES_IN_FLIGHT;
		init_info.MSAASamples = gfxDevice->m_MsaaSamples;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, gfxDevice->defaultRenderPass);

		VkCommandBuffer commandBuffer = gfxDevice->BeginSingleTimeCommandBuffer();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		gfxDevice->EndSingleTimeCommandBuffer(commandBuffer);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
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

	void UI::Shutdown(LogicalDevice& logicalDevice) {
		std::cout << "Destroying UI" << '\n';

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(logicalDevice.GetHandle(), m_UIDescriptorPool, nullptr);
		vkDestroyCommandPool(logicalDevice.GetHandle(), m_UICommandPool, nullptr);
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