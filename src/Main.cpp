/*
	Future Challenges

	- Implement a dynamic uniform buffer 

	- Use a different queue family specifically for transfer operations
		It will require the following modifications to the program:

		- Modify QueueFamilyIndices and findQueueFamilies to explicitly look for a queue family with the
		VK_QUEUE_TRANSFER_BIT bit, but not the VK_QUEUE_GRAPHICS_BIT.
		- Modify createLogicalDevice to request a handle to the transfer queue.
		- Create a second command pool for command buffers that are submitted on the transfer queue family.
		- Submit any transfer commands like vkCmdCopyBuffer to the transfer queue instead of the graphics queue.

	- Proper buffer memory allocation
		In a real world application, we're not supposed to actually call vkAllocateMemory for every individual
		buffer. The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount
		physical device limit, which may be as low as 4096 even on high end hardware. The right way to allocate
		memory for a large number of objects at the same time is to create a custom allocator that splits up a
		single allocation among many different objects by using the offset parameters that we've seen in many
		functions.

	- Multiple buffers into a single VkBuffer
		The Driver developers from Vulkan recommend that we also store multiple buffers, like the vertex and
		index buffer, into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers. The
		advantage is that our data is more cache friendly in that case, because it's closer together. It is
		even possible to reuse the same chunk of memory for multiple resources if they are not used during
		the same render operations, provided that their data is refreshed, of course. This is known as
		aliasing and some Vulkan functions have explicit flags to specify that you want to do this.

*/
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <random>
#include <functional>

#include "Application.h"
#include "CustomPipelineLayout.h"
#include "RenderLayout.h"
#include "../Assets/Scene.h"
#include "../Assets/Model.h"

class MyQuadOne : public Assets::Model {
public:
	void OnCreate() {
		std::vector<Assets::Vertex> v = { 
			{ {-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
			{ {0.0f, -1.0f}, {0.0f, 1.0f, 0.0f} },
			{ {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
			{ {-1.0f, 0.0f}, {1.0f, 1.0f, 1.0f} }
		};

		std::vector<uint16_t> i = {
			0, 1, 2, 2, 3, 0
		};

		SetVertices(v);
		SetIndices(i);
		m_ID = "Quad One";

		std::cout << m_ID << " model created\n";
	}

	void OnUIRender() {
		std::string modelId = m_ID;

		std::string t = "Transformations " + modelId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + modelId;
			std::string t_label_y = "Translation y " + modelId;
			std::string t_label_z = "Translation z " + modelId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, 0.0f, 1.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, 0.0f, 1.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, 0.0f, 1.0f);
			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
	}
};

class MyQuadTwo : public Assets::Model {
public:
	void OnCreate() {
		std::vector<Assets::Vertex> v = { 
			{ {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
			{ {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
			{ {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
			{ {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} }
		};

		std::vector<uint16_t> i = {
			0, 1, 2, 2, 3, 0
		};

		SetVertices(v);
		SetIndices(i);
		m_ID = "Quad Two";
		
		std::cout << m_ID << " model created\n";
	}

	void OnUIRender() {
		std::string modelId = m_ID;

		std::string t = "Transformations " + modelId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + modelId;
			std::string t_label_y = "Translation y " + modelId;
			std::string t_label_z = "Translation z " + modelId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, 0.0f, 1.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, 0.0f, 1.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, 0.0f, 1.0f);
			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {

	}
};

class MyScene : public Assets::Scene {
public:
	void OnCreate() {
		for (auto model : GetSceneModels()) {
			model->OnCreate();
		}
	}

	void OnUIRender() {
		for (auto model : GetSceneModels()) {
			model->OnUIRender();
		}
	}

	void OnUpdate(float t) {
		for (auto model : GetSceneModels()) {
			model->OnUpdate(t);
		}
	}
};

int main() {

	auto q1 = MyQuadOne();
	auto q2 = MyQuadTwo();

	MyScene* myScene = new MyScene();
	myScene->AddModel(&q1);
	myScene->AddModel(&q2);
	
	myScene->OnCreate();
	
	Engine::RenderLayout* renderLayout = new Engine::RenderLayout();

	Engine::GraphicsPipelineLayout defaultLayout;

	defaultLayout.bindingDescription = Assets::Vertex::getBindingDescription();
	defaultLayout.attributeDescriptions = Assets::Vertex::getAttributeDescriptions();
	defaultLayout.vertexShaderPath = "./Assets/Shaders/vert.spv";
	defaultLayout.fragmentShaderPath = "./Assets/Shaders/frag.spv";
	defaultLayout.maxDescriptorSets = myScene->GetSceneModels().size();

	renderLayout->AddGraphicsPipelineLayout(defaultLayout);

	Engine::WindowSettings windowSettings;
	windowSettings.Title = "VulkanApplication.exe";
	windowSettings.Width = 800;
	windowSettings.Height = 600;

	Engine::UserSettings userSettings;
	userSettings.rayTraced = false;
	
	Engine::Application* app = new Engine::Application(windowSettings, userSettings, renderLayout);

	app->SetActiveScene(myScene);
	app->Init();

	try {
		app->Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	delete myScene;
	delete app;

	return EXIT_SUCCESS;
}
