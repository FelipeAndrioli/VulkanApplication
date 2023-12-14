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

	- Asynchronous Command Buffer
		Create a setupCommandBuffer that the helper functions record commands into, and add a flushSetupCommands 
		to execute the commands that have been recorded so far. It's best to do this after the texture mapping 
		works to check if the texture resources are still set up correctly.

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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Application.h"
#include "MaterialLayout.h"
#include "../Assets/Scene.h"
#include "../Assets/Object.h"

class MyCubeOne : public Assets::Object {
public:
	struct UniformBufferObject {
		glm::mat4 object = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);
	};

	UniformBufferObject *ubo = new UniformBufferObject();

	void OnCreate() {

		SetUniformBufferObject(ubo, sizeof(*ubo));

		std::vector<Assets::Vertex> v = {
			{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }},		// top front right		0
			{{ 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }},		// top back right		1
			{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }},		// top front left		2
			{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }},		// top back left		3
			{{ -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }},		// bottom front left	4
			{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }},	// bottom back left		5
			{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }},		// bottom front right	6
			{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }}		// bottom back right	7
		};

		std::vector<uint16_t> i = {
			0, 2, 6, 2, 4, 6,		// front face
			0, 6, 7, 0, 1, 7,		// right face
			3, 4, 2, 3, 5, 4,		// left face
			3, 5, 7, 3, 7, 1,		// bottom face
			2, 3, 0, 3, 0, 1,		// top face
			5, 4, 6, 5, 6, 7		// bottom face
		};

		m_Transform.translation.z = -4.0f;

		AddVertices(v);
		AddIndices(i);
		ID = "Quad One";

		std::cout << ID << " object created\n";
	}

	void OnUIRender() {
		std::string objectId = ID;

		std::string t = "Transformations " + objectId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + objectId;
			std::string t_label_y = "Translation y " + objectId;
			std::string t_label_z = "Translation z " + objectId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, -20.0f, 20.0f);

			std::string r_label_x = "Rotation x " + objectId;
			std::string r_label_y = "Rotation y " + objectId;
			std::string r_label_z = "Rotation z " + objectId;
			
			ImGui::SliderFloat(r_label_x.c_str(), &m_Transform.rotation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &m_Transform.rotation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &m_Transform.rotation.z, -20.0f, 20.0f);

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		m_Transform.rotation.x += t * 0.05f;

		ubo->object = GetModelMatrix();
		ubo->view = SceneCamera->ViewMatrix;
		ubo->proj = SceneCamera->ProjectionMatrix;
	}
};

class MyCubeTwo : public Assets::Object {
public:

	struct UniformBufferObject {
		glm::mat4 object = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);

		glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);
	};

	UniformBufferObject *ubo = new UniformBufferObject();

	void OnCreate() {

		SetUniformBufferObject(ubo, sizeof(*ubo));

		std::vector<Assets::Vertex> v = {
			{{ 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// top front right		0
			{{ 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }},		// top back right		1
			{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// top front left		2
			{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }},		// top back left		3
			{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// bottom front left	4
			{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }},	// bottom back left		5
			{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// bottom front right	6
			{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }}		// bottom back right	7
		};

		std::vector<uint16_t> i = {
			0, 2, 6, 2, 4, 6,		// front face
			0, 6, 7, 0, 1, 7,		// right face
			3, 4, 2, 3, 5, 4,		// left face
			3, 5, 7, 3, 7, 1,		// bottom face
			2, 3, 0, 3, 0, 1,		// top face
			5, 4, 6, 5, 6, 7		// bottom face
		};

		m_Transform.translation.x = 2.0f;
		m_Transform.translation.z = -4.0f;

		AddVertices(v);
		AddIndices(i);
		ID = "Quad Two";
		
		std::cout << ID << " object created\n";
	}

	void OnUIRender() {
		std::string objectId = ID;

		std::string t = "Transformations " + objectId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + objectId;
			std::string t_label_y = "Translation y " + objectId;
			std::string t_label_z = "Translation z " + objectId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, -20.0f, 20.0f);

			std::string r_label_x = "Rotation x " + objectId;
			std::string r_label_y = "Rotation y " + objectId;
			std::string r_label_z = "Rotation z " + objectId;
			
			ImGui::SliderFloat(r_label_x.c_str(), &m_Transform.rotation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &m_Transform.rotation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &m_Transform.rotation.z, -20.0f, 20.0f);
			
			ImGui::ColorEdit3("Color", glm::value_ptr(ubo->color));

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		m_Transform.rotation.y += t * 0.05f;

		ubo->object = GetModelMatrix();
		ubo->view = SceneCamera->ViewMatrix;
		ubo->proj = SceneCamera->ProjectionMatrix;
	}
};

class MyCubeThree : public Assets::Object {
public:

	struct UniformBufferObject {
		glm::mat4 object = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 proj = glm::mat4(1.0f);
	};

	UniformBufferObject *ubo = new UniformBufferObject();

	void OnCreate() {

		SetUniformBufferObject(ubo, sizeof(*ubo));

		std::vector<Assets::Vertex> v = {
			{{ 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// top front right		0
			{{ 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }},		// top back right		1
			{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// top front left		2
			{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }},		// top back left		3
			{{ -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// bottom front left	4
			{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }},	// bottom back left		5
			{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f }},		// bottom front right	6
			{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }}		// bottom back right	7
		};

		std::vector<uint16_t> i = {
			0, 2, 6, 2, 4, 6,		// front face
			0, 6, 7, 0, 1, 7,		// right face
			3, 4, 2, 3, 5, 4,		// left face
			3, 5, 7, 3, 7, 1,		// bottom face
			2, 3, 0, 3, 0, 1,		// top face
			5, 4, 6, 5, 6, 7		// bottom face
		};

		m_Transform.translation.x = -2.0f;
		m_Transform.translation.z = -4.0f;

		AddVertices(v);
		AddIndices(i);
		ID = "Quad Three";
		
		std::cout << ID << " object created\n";
	}

	void OnUIRender() {
		std::string objectId = ID;

		std::string t = "Transformations " + objectId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + objectId;
			std::string t_label_y = "Translation y " + objectId;
			std::string t_label_z = "Translation z " + objectId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, -20.0f, 20.0f);

			std::string r_label_x = "Rotation x " + objectId;
			std::string r_label_y = "Rotation y " + objectId;
			std::string r_label_z = "Rotation z " + objectId;
			
			ImGui::SliderFloat(r_label_x.c_str(), &m_Transform.rotation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &m_Transform.rotation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &m_Transform.rotation.z, -20.0f, 20.0f);

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		m_Transform.rotation.z += t * 0.05f;

		ubo->object = GetModelMatrix();
		ubo->view = SceneCamera->ViewMatrix;
		ubo->proj = SceneCamera->ProjectionMatrix;
	}
};

int main() {

	std::unique_ptr<Engine::Material> rainbowMaterial = std::make_unique<Engine::Material>();
	rainbowMaterial->Layout.ID = "RainbowMaterial";
	rainbowMaterial->Layout.VertexShaderPath = "./Assets/Shaders/vert.spv";
	rainbowMaterial->Layout.FragmentShaderPath = "./Assets/Shaders/frag.spv";

	std::unique_ptr<Engine::Material> colorMaterial = std::make_unique<Engine::Material>();
	colorMaterial->Layout.ID = "ColorMaterial";
	colorMaterial->Layout.VertexShaderPath = "./Assets/Shaders/shader_test_vert.spv";
	colorMaterial->Layout.FragmentShaderPath = "./Assets/Shaders/shader_test_frag.spv";

	std::unique_ptr<Assets::Scene> myScene = std::make_unique<Assets::Scene>();

	myScene->AddMaterial(rainbowMaterial.get());
	myScene->AddMaterial(colorMaterial.get());

	auto q1 = MyCubeOne();
	q1.Material = rainbowMaterial.get();
	q1.SceneCamera = &myScene->DefaultCamera;
	auto q2 = MyCubeTwo();
	q2.SceneCamera = &myScene->DefaultCamera;
	q2.Material = colorMaterial.get();
	auto q3 = MyCubeThree();
	q3.SceneCamera = &myScene->DefaultCamera;

	myScene->AddObject(&q1);
	myScene->AddObject(&q2);
	myScene->AddObject(&q3);
	
	Engine::WindowSettings windowSettings;
	windowSettings.Title = "VulkanApplication.exe";
	windowSettings.Width = 800;
	windowSettings.Height = 600;

	Engine::UserSettings userSettings;
	userSettings.rayTraced = false;

	// TODO: review this initial settings
	std::unique_ptr<Engine::Application> app = std::make_unique<Engine::Application>(windowSettings, userSettings);

	app->SetActiveScene(myScene.get());
	app->Init();

	try {
		app->Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	rainbowMaterial.reset();
	colorMaterial.reset();
	
	myScene.reset();
	app.reset();

	return EXIT_SUCCESS;
}
