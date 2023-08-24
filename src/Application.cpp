#include "Application.h" 

static bool g_FramebufferResized = false;

static VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
static int g_MinImageCount = 2;

static Engine::UI g_UI;

const uint32_t PARTICLE_COUNT = 8192;

namespace Engine {
	Application::Application(const WindowSettings &windowSettings, const UserSettings &userSettings) : m_WindowSettings(windowSettings), m_UserSettings(userSettings) {
		m_Window.reset(new class Window(m_WindowSettings));
		m_Instance.reset(new class Instance(c_ValidationLayers, c_EnableValidationLayers));
		m_DebugMessenger.reset(c_EnableValidationLayers ? new class DebugUtilsMessenger(m_Instance->GetHandle()) : nullptr);

		Init();
	}

	Application::~Application() {
		Shutdown();
	}

	void Application::updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), 
			m_SwapChain->GetSwapChainExtent().width / (float)m_SwapChain->GetSwapChainExtent().height, 0.1f, 10.0f);

		// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way
		// to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. If we don't 
		// do this the image will be rendered upside down
		ubo.proj[1][1] *= -1;

		memcpy(m_UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	/* Disabled due to the new architecture, will think in a way to make it work later
	void Application::drawRayTraced(VkCommandBuffer &r_CommandBuffer, uint32_t imageIndex) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		vkWaitForFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame), VK_TRUE, UINT64_MAX);
		updateUniformBuffer(m_CurrentFrame);
		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame));

		vkResetCommandBuffer(m_ComputeCommandBuffers[m_CurrentFrame], 0);
		recordComputeCommandBuffer(m_ComputeCommandBuffers[m_CurrentFrame]);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_ComputeCommandBuffers[m_CurrentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = m_ComputeFinishedSemaphores->GetHandle(m_CurrentFrame);

		if (vkQueueSubmit(m_LogicalDevice->GetComputeQueue(), 1, &submitInfo, *m_ComputeInFlightFences->GetHandle(m_CurrentFrame)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit compute command buffer!");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(r_CommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass->GetHandle();
		renderPassInfo.framebuffer = m_SwapChain->GetSwapChainFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(r_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(r_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RayTracerGraphicsPipeline);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChain->GetSwapChainExtent().width;
		viewport.height = (float)m_SwapChain->GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(r_CommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChain->GetSwapChainExtent();

		vkCmdSetScissor(r_CommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(r_CommandBuffer, 0, 1, &m_ShaderStorageBuffers[m_CurrentFrame], offsets);

		vkCmdDraw(r_CommandBuffer, PARTICLE_COUNT, 1, 0, 0);

		vkCmdEndRenderPass(r_CommandBuffer);

		if (vkEndCommandBuffer(r_CommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}
	}
	*/

	void Application::drawRasterized(VkCommandBuffer &r_CommandBuffer, uint32_t imageIndex) {
		updateUniformBuffer(m_CurrentFrame);
		updateVertexBuffer(m_CurrentFrame);
		
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(r_CommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_GraphicsPipeline->GetRenderPass().GetHandle();
		renderPassInfo.framebuffer = m_SwapChain->GetSwapChainFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(r_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(r_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetHandle());

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChain->GetSwapChainExtent().width;
		viewport.height = (float)m_SwapChain->GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(r_CommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChain->GetSwapChainExtent();

		vkCmdSetScissor(r_CommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(r_CommandBuffer, 0, 1, &m_VertexBuffers[m_CurrentFrame], offsets);
		vkCmdBindIndexBuffer(r_CommandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(r_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetPipelineLayout().GetHandle(),
			0, 1, &m_DescriptorSets[m_CurrentFrame], 0, nullptr);

		vkCmdDrawIndexed(r_CommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(r_CommandBuffer);

		if (vkEndCommandBuffer(r_CommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

	void Application::handleDraw(uint32_t imageIndex) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(m_CurrentFrame));

		//m_UserSettings.IsRayTraced ? drawRayTraced(m_CommandBuffers[m_CurrentFrame], imageIndex) : 
		//	drawRasterized(m_CommandBuffers[m_CurrentFrame], imageIndex);

		drawRasterized(m_CommandBuffers[m_CurrentFrame], imageIndex);

		//vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);
		//recordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);
		g_UI.RecordCommands(m_SwapChain->GetSwapChainExtent(), m_CurrentFrame, imageIndex);

		VkSemaphore waitSemaphores[] = { *m_ComputeFinishedSemaphores->GetHandle(m_CurrentFrame), *m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame)};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		std::array<VkCommandBuffer, 2> cmdBuffers = { m_CommandBuffers[m_CurrentFrame], g_UI.GetCommandBuffer(m_CurrentFrame) };

		submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame);

		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submitInfo.pCommandBuffers = cmdBuffers.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = m_RenderFinishedSemaphores->GetHandle(m_CurrentFrame);
	
		if (vkQueueSubmit(m_LogicalDevice->GetGraphicsQueue(), 1, &submitInfo, *m_InFlightFences->GetHandle(m_CurrentFrame)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}
	}

	void Application::drawFrame() {

		g_UI.DrawUserSettings(m_UserSettings);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		uint32_t imageIndex;
		VkResult result;

		vkWaitForFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(m_CurrentFrame), VK_TRUE, UINT64_MAX);
		result = vkAcquireNextImageKHR(m_LogicalDevice->GetHandle(), m_SwapChain->GetHandle(), UINT64_MAX,
			*m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame), VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		handleDraw(imageIndex);

		// Present submit
		{
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = m_RenderFinishedSemaphores->GetHandle(m_CurrentFrame);
			
			VkSwapchainKHR swapChains[] = { m_SwapChain->GetHandle() };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;
		
			result = vkQueuePresentKHR(m_LogicalDevice->GetPresentQueue(), &presentInfo);
			
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || g_FramebufferResized) {
				g_FramebufferResized = false;
				recreateSwapChain();
				//return;
			} else if (result != VK_SUCCESS) {
				throw std::runtime_error("Failed to present swap chain image!");
			}
		}

		// using modulo operator to ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	
	void Application::Run() {

		m_Window->DrawFrame = std::bind(&Application::drawFrame, this);
		m_Window->OnKeyPress = std::bind(&Application::processKey, this, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4);
		m_Window->OnResize = std::bind(&Application::processResize, this, std::placeholders::_1, std::placeholders::_2);
		m_Window->Run();

		m_LogicalDevice->WaitIdle();
	}

	void Application::Init() {
		InitVulkan();

		// TODO: clean a bit this GUI initializer
		g_UI.Init(*m_Window->GetHandle(), m_Instance->GetHandle(), m_PhysicalDevice->GetHandle(),
			m_LogicalDevice->GetHandle(), m_PhysicalDevice->GetQueueFamilyIndices(), m_LogicalDevice->GetGraphicsQueue(),
			m_SwapChain->GetSwapChainExtent(), m_SwapChain->GetSwapChainImageViews(), 
			m_SwapChain->GetSwapChainImageFormat(), g_MinImageCount);
	}

	void Application::processResize(int width, int height) {
		g_FramebufferResized = true;
		std::cout << "width - " << width << " height - " << height << '\n';
	}

	void Application::processKey(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			std::cout << "Closing application" << '\n';
			m_Window->Close();
		}
	}

	void Application::InitVulkan() {
		m_Surface.reset(new class Surface(m_Instance->GetHandle(), *m_Window->GetHandle()));
		m_PhysicalDevice.reset(new class PhysicalDevice(m_Instance->GetHandle(), m_Surface->GetHandle()));
		m_LogicalDevice.reset(new class LogicalDevice(m_Instance.get(), m_PhysicalDevice.get()));

		m_SwapChain.reset(new class SwapChain(m_PhysicalDevice.get(), m_Window.get(), m_LogicalDevice.get(), m_Surface->GetHandle()));

		createComputeDescriptorSetLayout();
		m_GraphicsPipeline.reset(new class GraphicsPipeline("./Assets/Shaders/vert.spv", "Assets/Shaders/frag.spv",
			m_LogicalDevice.get(), m_SwapChain.get()));
		
		m_SwapChain->CreateFramebuffers(m_GraphicsPipeline->GetRenderPass().GetHandle());

		/*
		createGraphicsPipeline("./Assets/Shaders/particle_shader_vert.spv", "./Assets/Shaders/particle_shader_frag.spv",
			Particle::getBindindDescription(), Particle::getAttributeDescriptions(), 
			VK_PRIMITIVE_TOPOLOGY_POINT_LIST, m_RayTracerPipelineLayout, m_RayTracerGraphicsPipeline, 
			0, m_DescriptorSetLayout);
		createGraphicsPipeline("./Assets/Shaders/vert.spv", "./Assets/Shaders/frag.spv",
			Vertex::getBindingDescription(), Vertex::getAttributeDescriptions(), 
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, m_RasterizerPipelineLayout, m_RasterizerGraphicsPipeline, 
			1, m_DescriptorSetLayout);
		*/
		createComputePipeline();
		createCommandPool();
		createVertexBuffer();
		createIndexBuffer();
		createShaderStorageBuffers();
		createUniformBuffers();
		createDescriptorPool();
		createComputeDescriptorPool();
		createComputeDescriptorSets();
		createDescriptorSets();
		createCommandBuffers();
		createComputeCommandBuffers();
		createSyncObjects();
	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	VkShaderModule createShaderModule(const std::vector<char>& code, LogicalDevice* p_LogicalDevice) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(p_LogicalDevice->GetHandle(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	void Application::createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = m_PhysicalDevice->GetQueueFamilyIndices();

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(m_LogicalDevice->GetHandle(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Command Pool!");
		}
	}

	uint32_t findMemoryType(VkPhysicalDevice& physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_LogicalDevice->GetHandle(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_LogicalDevice->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_LogicalDevice->GetGraphicsQueue());

		vkFreeCommandBuffers(m_LogicalDevice->GetHandle(), m_CommandPool, 1, &commandBuffer);
	}

	void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(m_LogicalDevice->GetHandle(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_LogicalDevice->GetHandle(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(m_PhysicalDevice->GetHandle(), memRequirements.memoryTypeBits, properties);
	
		if (vkAllocateMemory(m_LogicalDevice->GetHandle(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(m_LogicalDevice->GetHandle(), buffer, bufferMemory, 0);
	}

	void Application::createVertexBuffer() {
		m_VertexBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_VertexBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffers[i], m_VertexBuffersMemory[i]);
		}
	}

	void Application::updateVertexBuffer(uint32_t currentImage) {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory);
		
		copyBuffer(stagingBuffer, m_VertexBuffers[currentImage], bufferSize);

		vkDestroyBuffer(m_LogicalDevice->GetHandle(), stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, nullptr);
	}

	void Application::createIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);
		
		copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(m_LogicalDevice->GetHandle(), stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, nullptr);
	}

	void Application::createShaderStorageBuffers() {
		std::default_random_engine rndEngine((unsigned)time(nullptr));
		std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

		std::vector<Particle> particles(PARTICLE_COUNT);
		
		for (auto& particle : particles) {
			float r = 0.25f * sqrt(rndDist(rndEngine));
			float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
			float x = r * cos(theta) * m_SwapChain->GetSwapChainExtent().height / m_SwapChain->GetSwapChainExtent().height;
			float y = r * sin(theta);
			particle.position = glm::vec2(x, y);
			particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
			particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
		}

		VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, particles.data(), (size_t)bufferSize);
		vkUnmapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory);

		m_ShaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_ShaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_ShaderStorageBuffers[i], m_ShaderStorageBuffersMemory[i]);
			copyBuffer(stagingBuffer, m_ShaderStorageBuffers[i], bufferSize);
		}

		vkDestroyBuffer(m_LogicalDevice->GetHandle(), stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, nullptr);
	}

	void Application::createComputeDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_ComputeDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_ComputeDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		m_ComputeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		if (vkAllocateDescriptorSets(m_LogicalDevice->GetHandle(), &allocInfo, m_ComputeDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate compute descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo uniformBufferInfo = {};
			uniformBufferInfo.buffer = m_UniformBuffers[i];
			uniformBufferInfo.offset = 0;
			uniformBufferInfo.range = sizeof(UniformBufferObject);

			std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_ComputeDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

			VkDescriptorBufferInfo storageBufferInfoLastFrame = {};
			storageBufferInfoLastFrame.buffer = m_ShaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
			storageBufferInfoLastFrame.offset = 0;
			storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_ComputeDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

			VkDescriptorBufferInfo storageBufferInfoCurrentFrame = {};
			storageBufferInfoCurrentFrame.buffer = m_ShaderStorageBuffers[i];
			storageBufferInfoCurrentFrame.offset = 0;
			storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = m_ComputeDescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

			vkUpdateDescriptorSets(m_LogicalDevice->GetHandle(), 3, descriptorWrites.data(), 0, nullptr);
		}
	}

	void Application::createComputePipeline() {
		auto computeShaderCode = readFile("./Assets/Shaders/particle_shader_comp.spv");

		VkShaderModule computeShaderModule = createShaderModule(computeShaderCode, m_LogicalDevice.get());

		VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = computeShaderModule;
		computeShaderStageInfo.pName = "main";

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_ComputeDescriptorSetLayout;

		if (vkCreatePipelineLayout(m_LogicalDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &m_ComputePipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Compute pipeline layout!");
		}

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = m_ComputePipelineLayout;
		pipelineInfo.stage = computeShaderStageInfo;

		if (vkCreateComputePipelines(m_LogicalDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_ComputePipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Compute pipeline!");
		}

		vkDestroyShaderModule(m_LogicalDevice->GetHandle(), computeShaderModule, nullptr);
	}

	void Application::createComputeDescriptorSetLayout() {
		std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings = {};
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBindings[0].pImmutableSamplers = nullptr;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorCount = 1;
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindings[1].pImmutableSamplers = nullptr;
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		layoutBindings[2].binding = 2;
		layoutBindings[2].descriptorCount = 1;
		layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindings[2].pImmutableSamplers = nullptr;
		layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 3;
		layoutInfo.pBindings = layoutBindings.data();
	
		if (vkCreateDescriptorSetLayout(m_LogicalDevice->GetHandle(), &layoutInfo, nullptr, &m_ComputeDescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Compute descriptor set layout!");
		}
	}

	void Application::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1,
			&m_ComputeDescriptorSets[m_CurrentFrame], 0, nullptr);
		vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record compute command buffer!");
		}
	}

	void Application::createComputeCommandBuffers() {
		m_ComputeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_ComputeCommandBuffers.size();

		if (vkAllocateCommandBuffers(m_LogicalDevice->GetHandle(), &allocInfo, m_ComputeCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate compute command buffers!");
		}
	}

	void Application::createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
	
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				m_UniformBuffers[i], m_UniformBuffersMemory[i]);

			vkMapMemory(m_LogicalDevice->GetHandle(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
		}
	}

	void Application::createComputeDescriptorPool() {
		// TODO change the create descriptor pool function to receive the descriptor pool as parameter

		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		/*
			We need to double the number of VK_DESCRIPTOR_TYPE_STORAGE_BUFFER types requested from the pool
			because our sets reference the SSBOs tof the last and current frame (for now).
		*/
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolInfo.flags = 0;

		if (vkCreateDescriptorPool(m_LogicalDevice->GetHandle(), &poolInfo, nullptr, &m_ComputeDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Descriptor Pool!");
		}
	}

	void Application::createDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		/*
			We need to double the number of VK_DESCRIPTOR_TYPE_STORAGE_BUFFER types requested from the pool
			because our sets reference the SSBOs tof the last and current frame (for now).
		*/
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolInfo.flags = 0;

		if (vkCreateDescriptorPool(m_LogicalDevice->GetHandle(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Descriptor Pool!");
		}
	}

	void Application::createDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_GraphicsPipeline->GetDescriptorSetLayout().GetHandle());

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		if (vkAllocateDescriptorSets(m_LogicalDevice->GetHandle(), &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate Descriptor Sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(m_LogicalDevice->GetHandle(), 1, &descriptorWrite, 0, nullptr);
		}
	}

	void Application::createCommandBuffers() {
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();
	
		if (vkAllocateCommandBuffers(m_LogicalDevice->GetHandle(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Command Buffer!");
		}
	}

	void Application::createSyncObjects() {
		// TODO: The Semaphore and Fence classes are pretty similar, maybe we can turn them into a template instead of two different classes

		m_ImageAvailableSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_RenderFinishedSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_ComputeFinishedSemaphores.reset(new class Semaphore(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));

		m_InFlightFences.reset(new class Fence(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
		m_ComputeInFlightFences.reset(new class Fence(m_LogicalDevice->GetHandle(), MAX_FRAMES_IN_FLIGHT));
	}

	void Application::recreateSwapChain() {

		m_SwapChain->ReCreate(m_GraphicsPipeline->GetRenderPass().GetHandle());
		createCommandBuffers();
		g_UI.Resize(m_LogicalDevice->GetHandle(), m_SwapChain->GetSwapChainExtent(), 
			m_SwapChain->GetSwapChainImageViews(), g_MinImageCount);
	}

	void Application::Shutdown() {
		g_UI.Destroy(m_LogicalDevice->GetHandle());
		m_SwapChain.reset();

		vkDestroyPipeline(m_LogicalDevice->GetHandle(), m_ComputePipeline, nullptr);
		vkDestroyPipelineLayout(m_LogicalDevice->GetHandle(), m_ComputePipelineLayout, nullptr);
		vkDestroyDescriptorPool(m_LogicalDevice->GetHandle(), m_ComputeDescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(m_LogicalDevice->GetHandle(), m_ComputeDescriptorSetLayout, nullptr);

		//vkDestroyPipeline(m_LogicalDevice->GetHandle(), m_RayTracerGraphicsPipeline, nullptr);
		//vkDestroyPipelineLayout(m_LogicalDevice->GetHandle(), m_RayTracerPipelineLayout, nullptr);
		m_GraphicsPipeline.reset();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(m_LogicalDevice->GetHandle(), m_UniformBuffers[i], nullptr);
			vkFreeMemory(m_LogicalDevice->GetHandle(), m_UniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(m_LogicalDevice->GetHandle(), m_DescriptorPool, nullptr);
		
		vkDestroyBuffer(m_LogicalDevice->GetHandle(), m_IndexBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice->GetHandle(), m_IndexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(m_LogicalDevice->GetHandle(), m_ShaderStorageBuffers[i], nullptr);
			vkFreeMemory(m_LogicalDevice->GetHandle(), m_ShaderStorageBuffersMemory[i], nullptr);
		}

		for (size_t i = 0; i < m_VertexBuffers.size(); i++) {
			vkDestroyBuffer(m_LogicalDevice->GetHandle(), m_VertexBuffers[i], nullptr);
			vkFreeMemory(m_LogicalDevice->GetHandle(), m_VertexBuffersMemory[i], nullptr);
		}

		m_DebugMessenger.reset();

		m_InFlightFences.reset();
		m_ComputeInFlightFences.reset();

		m_ImageAvailableSemaphores.reset();
		m_RenderFinishedSemaphores.reset();
		m_ComputeFinishedSemaphores.reset();

		vkDestroyCommandPool(m_LogicalDevice->GetHandle(), m_CommandPool, nullptr);

		m_LogicalDevice.reset();
		m_Surface.reset();
		m_Instance.reset();
		
		glfwTerminate();
	}
}
