#include "ApplicationCore.h"

namespace Engine {

	ApplicationCore::ApplicationCore(const Settings &settings) : m_Settings(settings) {
		m_Window.reset(new class Window(m_Settings));
		m_Input.reset(new class InputSystem::Input());

		//m_Window->Render = std::bind(&Application::Draw, this);
		m_Window->OnKeyPress = std::bind(&InputSystem::Input::ProcessKey, m_Input.get(), std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3, std::placeholders::_4);
		m_Window->OnResize = std::bind(&ApplicationCore::Resize, this, std::placeholders::_1, std::placeholders::_2);
		m_Window->OnMouseClick = std::bind(&InputSystem::Input::ProcessMouseClick, m_Input.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		m_Window->OnCursorMove = std::bind(&InputSystem::Input::ProcessCursorMove, m_Input.get(), std::placeholders::_1, std::placeholders::_2);
		m_Window->OnCursorOnScreen = std::bind(&InputSystem::Input::ProcessCursorOnScreen, m_Input.get(), std::placeholders::_1);

		m_SceneGPUData = {};
	}
	
	ApplicationCore::~ApplicationCore() {

	}

	void ApplicationCore::RunApplication(IScene& scene) {
		InitializeApplication(scene);

		while (UpdateApplication(scene)) {
			glfwPollEvents();
		}

		TerminateApplication(scene);
	}

	bool ApplicationCore::UpdateApplication(IScene& scene) {
		m_CurrentFrameTime = (float)glfwGetTime();
		Timestep timestep = m_CurrentFrameTime - m_LastFrameTime;

		if (m_Settings.limitFramerate && timestep.GetSeconds() < (1 / 60.0f)) return true;

		scene.Update(timestep.GetMilliseconds(), *m_Input.get());
		scene.RenderScene();
		scene.RenderUI();

		m_LastFrameTime = m_CurrentFrameTime;

		m_Settings.ms = timestep.GetMilliseconds();
		m_Settings.frames = static_cast<float>(1 / timestep.GetSeconds());

		return scene.IsDone(*m_Input.get());
	}

	void ApplicationCore::InitializeApplication(IScene& scene) {
		scene.StartUp(*m_VulkanEngine.get());
	}

	void ApplicationCore::TerminateApplication(IScene& scene) {
		scene.CleanUp();
	}

	void ApplicationCore::Resize(int width, int height) {
		std::cout << "width - " << width << " height - " << height << '\n';
		//m_VulkanEngine->Resize();
		//p_ActiveScene->OnResize(width, height);
	}

	void ApplicationCore::RenderSkybox(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	}

	void ApplicationCore::RenderScene(const VkCommandBuffer& commandBuffer, const VkPipeline& graphicsPipeline, const std::vector<Assets::Object*>& objects) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		for (size_t i = 0; i < objects.size(); i++) {
			if (objects[i]->GetGraphicsPipeline()->GetHandle() != graphicsPipeline && graphicsPipeline != m_WireframePipeline->GetHandle())
				continue;

			Assets::Object* object = objects[i];

			object->DescriptorSets->Bind(
				m_CurrentFrame, 
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				m_MainPipelineLayout->GetHandle()
			);

			ObjectGPUData objectGPUData = ObjectGPUData();
			objectGPUData.model = object->GetModelMatrix();

			VkDeviceSize objectBufferOffset = i * m_GPUDataBuffer->Chunks[OBJECT_BUFFER_INDEX].DataSize;
			m_GPUDataBuffer->Update(m_CurrentFrame, objectBufferOffset, &objectGPUData, sizeof(ObjectGPUData));
	
			for (const auto& mesh : object->Meshes) {
				vkCmdPushConstants(
					commandBuffer,
					m_MainPipelineLayout->GetHandle(),
					VK_SHADER_STAGE_FRAGMENT_BIT,
					0,
					sizeof(int),
					&mesh.MaterialIndex
				);

				vkCmdDrawIndexed(
					commandBuffer,
					static_cast<uint32_t>(mesh.Indices.size()),
					1,
					static_cast<uint32_t>(mesh.IndexOffset),
					static_cast<int32_t>(mesh.VertexOffset),
					0
				);
			}
		}
	}

	void ApplicationCore::RenderModel(const VkCommandBuffer& commandBuffer, const Assets::Object& object) {
	
	}
}