#include "Application.h" 

static bool g_FramebufferResized = false;

static int g_MinImageCount = 2;
static Engine::UI g_UI;

namespace Engine {
	Application::Application(const WindowSettings &windowSettings, const UserSettings &userSettings) : m_WindowSettings(windowSettings), m_UserSettings(userSettings) {
		m_Window.reset(new class Window(&m_WindowSettings));
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

		memcpy(m_UniformBuffers->GetBufferMemoryMapped(currentImage), &ubo, sizeof(ubo));
	}

	/* Disabled due to the new architecture, will think in a way to make it work later*/
	void Application::drawRayTraced(VkCommandBuffer& p_CommandBuffer, uint32_t imageIndex) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		vkWaitForFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame), VK_TRUE, UINT64_MAX);
		updateUniformBuffer(m_CurrentFrame);
		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_ComputeInFlightFences->GetHandle(m_CurrentFrame));

		auto computeCommandBuffer = m_ComputeCommandBuffers->Begin(m_CurrentFrame);
		recordComputeCommandBuffer(computeCommandBuffer);
		m_ComputeCommandBuffers->End(m_CurrentFrame);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_ComputeCommandBuffers->GetCommandBuffer(m_CurrentFrame);
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = m_ComputeFinishedSemaphores->GetHandle(m_CurrentFrame);

		if (vkQueueSubmit(m_LogicalDevice->GetComputeQueue(), 1, &submitInfo, *m_ComputeInFlightFences->GetHandle(m_CurrentFrame)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit compute command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_TempRayTracerPipeline->GetRenderPass().GetHandle();
		renderPassInfo.framebuffer = m_SwapChain->GetSwapChainFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(p_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TempRayTracerPipeline->GetHandle());

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChain->GetSwapChainExtent().width;
		viewport.height = (float)m_SwapChain->GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(p_CommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChain->GetSwapChainExtent();

		vkCmdSetScissor(p_CommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1, &m_ShaderStorageBuffers->GetBuffer(m_CurrentFrame), offsets);
		vkCmdDraw(p_CommandBuffer, PARTICLE_COUNT, 1, 0, 0);
		vkCmdEndRenderPass(p_CommandBuffer);
	}

	void Application::drawRasterized(VkCommandBuffer& p_CommandBuffer, uint32_t imageIndex) {
		updateUniformBuffer(m_CurrentFrame);
		updateVertexBuffer(m_CurrentFrame);
		
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_GraphicsPipeline->GetRenderPass().GetHandle();
		renderPassInfo.framebuffer = m_SwapChain->GetSwapChainFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(p_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetHandle());

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChain->GetSwapChainExtent().width;
		viewport.height = (float)m_SwapChain->GetSwapChainExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(p_CommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapChain->GetSwapChainExtent();

		vkCmdSetScissor(p_CommandBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(p_CommandBuffer, 0, 1, &m_VertexBuffers->GetBuffer(m_CurrentFrame), offsets);
		vkCmdBindIndexBuffer(p_CommandBuffer, m_IndexBuffer->GetBuffer(0), 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(p_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetPipelineLayout().GetHandle(),
			0, 1, &m_GraphicsPipeline->GetDescriptorSets().GetDescriptorSet(m_CurrentFrame), 0, nullptr);
		vkCmdDrawIndexed(p_CommandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(p_CommandBuffer);
	}

	void Application::handleDraw(uint32_t imageIndex) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		
		vkResetFences(m_LogicalDevice->GetHandle(), 1, m_InFlightFences->GetHandle(m_CurrentFrame));

		auto commandBuffer = m_CommandBuffers->Begin(m_CurrentFrame);
		m_UserSettings.rayTraced ? drawRayTraced(commandBuffer, imageIndex) : 
			drawRasterized(commandBuffer, imageIndex);
		m_CommandBuffers->End(m_CurrentFrame);

		g_UI.RecordCommands(m_SwapChain->GetSwapChainExtent(), m_CurrentFrame, imageIndex);

		VkSemaphore waitSemaphores[] = { *m_ComputeFinishedSemaphores->GetHandle(m_CurrentFrame), *m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame)};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		std::array<VkCommandBuffer, 2> cmdBuffers = { m_CommandBuffers->GetCommandBuffer(m_CurrentFrame), g_UI.GetCommandBuffer(m_CurrentFrame) };

		submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
		if (m_UserSettings.rayTraced) {
			submitInfo.waitSemaphoreCount = 2;
			submitInfo.pWaitSemaphores = waitSemaphores;
		}
		else {
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = m_ImageAvailableSemaphores->GetHandle(m_CurrentFrame);
		}

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

		g_UI.Draw(m_UserSettings, m_WindowSettings);

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
			}
			else if (result != VK_SUCCESS) {
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

		// createUniformBuffers();
		{
			VkDeviceSize bufferSize = sizeof(UniformBufferObject);
			m_UniformBuffers.reset(new class Buffer(MAX_FRAMES_IN_FLIGHT, m_LogicalDevice.get(), m_PhysicalDevice.get(), bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
			m_UniformBuffers->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			m_UniformBuffers->MapMemory();
		}

		m_GraphicsPipeline.reset(new class GraphicsPipeline("./Assets/Shaders/vert.spv", "Assets/Shaders/frag.spv",
			m_LogicalDevice.get(), m_SwapChain.get(), Assets::Vertex::getBindingDescription(), Assets::Vertex::getAttributeDescriptions(),
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, m_UniformBuffers.get()));
		m_SwapChain->CreateFramebuffers(m_GraphicsPipeline->GetRenderPass().GetHandle());

		m_CommandPool.reset(new class CommandPool(m_LogicalDevice->GetHandle(), m_PhysicalDevice->GetQueueFamilyIndices()));
		
		//createShaderStorageBuffers();
		{
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
			
			/*
			Buffer stagingBuffer = Buffer(1, m_LogicalDevice.get(), m_PhysicalDevice.get(),
				bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			stagingBuffer.AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			*/

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				stagingBuffer, stagingBufferMemory);

			void* data;
			//stagingBuffer.MapMemory(&data);
			vkMapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, particles.data(), (size_t)bufferSize);
			//stagingBuffer.UnmapMemory();
			vkUnmapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory);

			m_ShaderStorageBuffers.reset(new class Buffer(MAX_FRAMES_IN_FLIGHT, m_LogicalDevice.get(),
				m_PhysicalDevice.get(), bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
			m_ShaderStorageBuffers->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_ShaderStorageBuffers->CopyFrom(stagingBuffer, bufferSize,
				m_CommandPool->GetHandle());

			vkDestroyBuffer(m_LogicalDevice->GetHandle(), stagingBuffer, nullptr);
			vkFreeMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, nullptr);
		}

		m_TempRayTracerPipeline.reset(new class GraphicsPipeline("./Assets/Shaders/particle_shader_vert.spv", "./Assets/Shaders/particle_shader_frag.spv",
			m_LogicalDevice.get(), m_SwapChain.get(), Particle::getBindindDescription(), Particle::getAttributeDescriptions(),
			VK_PRIMITIVE_TOPOLOGY_POINT_LIST, m_UniformBuffers.get()));
		m_ComputePipeline.reset(new class ComputePipeline("./Assets/Shaders/particle_shader_comp.spv", m_LogicalDevice.get(), 
			m_SwapChain.get(), m_UniformBuffers.get(), m_ShaderStorageBuffers.get()));

		//createVertexBuffer();
		{
			VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
			m_VertexBuffers.reset(new class Buffer(static_cast<size_t>(MAX_FRAMES_IN_FLIGHT), m_LogicalDevice.get(), m_PhysicalDevice.get(), bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
			m_VertexBuffers->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}

		//createIndexBuffer();
		{
			VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
			//Buffer stagingBuffer = Buffer(1, m_LogicalDevice.get(), m_PhysicalDevice.get(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			//stagingBuffer.AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory);

			void* data;
			//stagingBuffer.MapMemory(&data);
			vkMapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, 0, bufferSize, 0, &data);
			memcpy(data, indices.data(), (size_t)bufferSize);
			vkUnmapMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory);
			//stagingBuffer.UnmapMemory();
			
			m_IndexBuffer.reset(new class Buffer(1, m_LogicalDevice.get(), m_PhysicalDevice.get(),
				bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
			m_IndexBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_IndexBuffer->CopyFrom(stagingBuffer, bufferSize,
				m_CommandPool->GetHandle());

			vkDestroyBuffer(m_LogicalDevice->GetHandle(), stagingBuffer, nullptr);
			vkFreeMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, nullptr);
		}

		//createCommandBuffers();
		//createComputeCommandBuffers();
		m_CommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), 
			m_LogicalDevice->GetHandle()));
		m_ComputeCommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(), 
			m_LogicalDevice->GetHandle()));

		createSyncObjects();
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

	void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, 
		VkDeviceMemory& bufferMemory) {
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
			throw std::runtime_error("Failed to allocate buffer memory!");
		}

		vkBindBufferMemory(m_LogicalDevice->GetHandle(), buffer, bufferMemory, 0);
	}

	void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool->GetHandle();
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

		vkFreeCommandBuffers(m_LogicalDevice->GetHandle(), m_CommandPool->GetHandle(), 1, &commandBuffer);
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
		
		copyBuffer(stagingBuffer, m_VertexBuffers->GetBuffer(currentImage), bufferSize);

		vkDestroyBuffer(m_LogicalDevice->GetHandle(), stagingBuffer, nullptr);
		vkFreeMemory(m_LogicalDevice->GetHandle(), stagingBufferMemory, nullptr);
	}

	void Application::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline->GetHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline->GetPipelineLayout().GetHandle(), 
			0, 1,&m_ComputePipeline->GetDescriptorSets().GetDescriptorSet(m_CurrentFrame), 
			0, nullptr);
		vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);
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
		m_CommandBuffers.reset(new class CommandBuffer(MAX_FRAMES_IN_FLIGHT, m_CommandPool->GetHandle(),
			m_LogicalDevice->GetHandle()));
		g_UI.Resize(m_LogicalDevice->GetHandle(), m_SwapChain->GetSwapChainExtent(), 
			m_SwapChain->GetSwapChainImageViews(), g_MinImageCount);
	}

	void Application::Shutdown() {
		g_UI.Destroy(m_LogicalDevice->GetHandle());
		m_SwapChain.reset();

		m_ComputePipeline.reset();

		m_CommandBuffers.reset();
		m_ComputeCommandBuffers.reset();

		m_GraphicsPipeline.reset();
		m_TempRayTracerPipeline.reset();

		m_UniformBuffers.reset();
		m_IndexBuffer.reset();
		m_ShaderStorageBuffers.reset();
		m_VertexBuffers.reset();

		m_DebugMessenger.reset();
		m_InFlightFences.reset();
		m_ComputeInFlightFences.reset();

		m_ImageAvailableSemaphores.reset();
		m_RenderFinishedSemaphores.reset();
		m_ComputeFinishedSemaphores.reset();

		m_CommandPool.reset();

		m_LogicalDevice.reset();
		m_Surface.reset();
		m_Instance.reset();
		
		glfwTerminate();
	}
}
