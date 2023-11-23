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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Application.h"
#include "ResourceSetLayout.h"
#include "../Assets/Scene.h"
#include "../Assets/Model.h"

class MyQuadOne : public Assets::Model {
public:
	struct UniformBufferObject {
		glm::mat4 model = glm::mat4(1.0f);
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

		SetVertices(v);
		SetIndices(i);
		ID = "Quad One";

		std::cout << ID << " model created\n";
	}

	void OnUIRender() {
		std::string modelId = ID;

		std::string t = "Transformations " + modelId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + modelId;
			std::string t_label_y = "Translation y " + modelId;
			std::string t_label_z = "Translation z " + modelId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, -20.0f, 20.0f);

			std::string r_label_x = "Rotation x " + modelId;
			std::string r_label_y = "Rotation y " + modelId;
			std::string r_label_z = "Rotation z " + modelId;
			
			ImGui::SliderFloat(r_label_x.c_str(), &m_Transform.rotation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &m_Transform.rotation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &m_Transform.rotation.z, -20.0f, 20.0f);

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		m_Transform.rotation.x += t * 0.05f;
		m_Transform.rotation.y += t * 0.05f;
		m_Transform.rotation.z += t * 0.05f;

		ubo->model = GetModelMatrix();
		ubo->view = glm::lookAt(glm::vec3(0.0f, 0.1f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo->proj = glm::perspective(45.0f, 800 / (float)600, 0.1f, 10.0f);

		// GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way
		// to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. If we don't 
		// do this the image will be rendered upside down
		ubo->proj[1][1] *= -1;

	}
};

class MyQuadTwo : public Assets::Model {
public:

	struct UniformBufferObject {
		glm::mat4 model = glm::mat4(1.0f);
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

		SetVertices(v);
		SetIndices(i);
		ID = "Quad Two";
		
		std::cout << ID << " model created\n";
	}

	void OnUIRender() {
		std::string modelId = ID;

		std::string t = "Transformations " + modelId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + modelId;
			std::string t_label_y = "Translation y " + modelId;
			std::string t_label_z = "Translation z " + modelId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, -20.0f, 20.0f);

			std::string r_label_x = "Rotation x " + modelId;
			std::string r_label_y = "Rotation y " + modelId;
			std::string r_label_z = "Rotation z " + modelId;
			
			ImGui::SliderFloat(r_label_x.c_str(), &m_Transform.rotation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &m_Transform.rotation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &m_Transform.rotation.z, -20.0f, 20.0f);
			
			ImGui::ColorEdit3("Color", glm::value_ptr(ubo->color));

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		m_Transform.rotation.x += t * 0.05f;
		m_Transform.rotation.y += t * 0.05f;
		m_Transform.rotation.z += t * 0.05f;

		ubo->model = GetModelMatrix();
		ubo->view = glm::lookAt(glm::vec3(0.0f, 0.1f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo->proj = glm::perspective(glm::radians(45.0f), 800 / (float) 600, 0.1f, 10.0f);

		ubo->proj[1][1] *= -1;
	}
};

class MyQuadThree : public Assets::Model {
public:

	struct UniformBufferObject {
		glm::mat4 model = glm::mat4(1.0f);
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

		m_Transform.translation.x = -2.0f;
		m_Transform.translation.z = -4.0f;

		SetVertices(v);
		SetIndices(i);
		ID = "Quad Three";
		
		std::cout << ID << " model created\n";
	}

	void OnUIRender() {
		std::string modelId = ID;

		std::string t = "Transformations " + modelId;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + modelId;
			std::string t_label_y = "Translation y " + modelId;
			std::string t_label_z = "Translation z " + modelId;

			ImGui::SliderFloat(t_label_x.c_str(), &m_Transform.translation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &m_Transform.translation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &m_Transform.translation.z, -20.0f, 20.0f);

			std::string r_label_x = "Rotation x " + modelId;
			std::string r_label_y = "Rotation y " + modelId;
			std::string r_label_z = "Rotation z " + modelId;
			
			ImGui::SliderFloat(r_label_x.c_str(), &m_Transform.rotation.x, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &m_Transform.rotation.y, -20.0f, 20.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &m_Transform.rotation.z, -20.0f, 20.0f);

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		m_Transform.rotation.x += t * 0.05f;
		m_Transform.rotation.y += t * 0.05f;
		m_Transform.rotation.z += t * 0.05f;

		ubo->model = GetModelMatrix();
		ubo->view = glm::lookAt(glm::vec3(0.0f, 0.1f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo->proj = glm::perspective(glm::radians(45.0f), 800 / (float) 600, 0.1f, 10.0f);

		ubo->proj[1][1] *= -1;
	}
};

int main() {
	Engine::ResourceSetLayout rainbowLayout;

	rainbowLayout.ResourceSetIndex = 0;
	rainbowLayout.BindingDescription = Assets::Vertex::getBindingDescription();
	rainbowLayout.AttributeDescriptions = Assets::Vertex::getAttributeDescriptions();
	rainbowLayout.VertexShaderPath = "./Assets/Shaders/vert.spv";
	rainbowLayout.FragmentShaderPath = "./Assets/Shaders/frag.spv";

	Engine::ResourceSetLayout colorLayout;

	colorLayout.ResourceSetIndex = 1;
	colorLayout.BindingDescription = Assets::Vertex::getBindingDescription();
	colorLayout.AttributeDescriptions = Assets::Vertex::getAttributeDescriptions();
	colorLayout.VertexShaderPath = "./Assets/Shaders/shader_test_vert.spv";
	colorLayout.FragmentShaderPath = "./Assets/Shaders/shader_test_frag.spv";

	auto q1 = MyQuadOne();
	q1.ResourceSetIndex = rainbowLayout.ResourceSetIndex;
	auto q2 = MyQuadTwo();
	q2.ResourceSetIndex = colorLayout.ResourceSetIndex;
	auto q3 = MyQuadThree();

	Assets::Scene* myScene = new Assets::Scene();
	myScene->AddResourceSetLayout(&rainbowLayout);
	myScene->AddResourceSetLayout(&colorLayout);
	myScene->AddModel(&q1);
	myScene->AddModel(&q2);
	myScene->AddModel(&q3);
	
	Engine::WindowSettings windowSettings;
	windowSettings.Title = "VulkanApplication.exe";
	windowSettings.Width = 800;
	windowSettings.Height = 600;

	Engine::UserSettings userSettings;
	userSettings.rayTraced = false;
	
	Engine::Application* app = new Engine::Application(windowSettings, userSettings);

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
