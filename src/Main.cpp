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

	- MipMaps
		It is uncommon in practice to generate the mipmap levels at runtime. Usually they are pregenerated and
		stored in the texture file alongside the base level to improve loading speed. A better approach would be
		to implement resizing in software and load multiple levels from a file.

*/

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Application.h"
#include "UI.h"

#include "../Assets/Camera.h"
#include "../Assets/Scene.h"
#include "../Assets/Object.h"
#include "../Assets/Shader.h"
#include "../Assets/Pipeline.h"
#include "../Assets/Material.h"
#include "../Assets/Utils/MeshGenerator.h"

class CustomObject : public Assets::Object {
public:

	bool rotate = false;

	void OnCreate() {

	}

	void OnUIRender() {
		std::string t = "Transformations " + ID;

		if (ImGui::TreeNode(t.c_str())) {
			std::string t_label_x = "Translation x " + ID;
			std::string t_label_y = "Translation y " + ID;
			std::string t_label_z = "Translation z " + ID;

			ImGui::SliderFloat(t_label_x.c_str(), &Transformations.translation.x, -200.0f, 200.0f);
			ImGui::SliderFloat(t_label_y.c_str(), &Transformations.translation.y, -200.0f, 200.0f);
			ImGui::SliderFloat(t_label_z.c_str(), &Transformations.translation.z, -200.0f, 200.0f);

			std::string r_label_x = "Rotation x " + ID;
			std::string r_label_y = "Rotation y " + ID;
			std::string r_label_z = "Rotation z " + ID;
			
			ImGui::SliderFloat(r_label_x.c_str(), &Transformations.rotation.x, -200.0f, 200.0f);
			ImGui::SliderFloat(r_label_y.c_str(), &Transformations.rotation.y, -200.0f, 200.0f);
			ImGui::SliderFloat(r_label_z.c_str(), &Transformations.rotation.z, -200.0f, 200.0f);

			std::string s_label = "Scale Handler " + ID;
			ImGui::SliderFloat(s_label.c_str(), &Transformations.scaleHandler, 0.0f, 2.0f);

			ImGui::Checkbox("Rotate", &rotate);

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		if (rotate) {
			Transformations.rotation.y += 0.01f * t;

			if (Transformations.rotation.y > 360.0f)
				Transformations.rotation.y = 0.0f;
		}
	}
};

int main() {
	std::unique_ptr<Assets::Scene> myScene = std::make_unique<Assets::Scene>();
	myScene->SetCameraPosition({ 12, 5, 13 }, -131, -28);


	CustomObject plane = CustomObject();
	//model.SetMesh(Assets::MeshGenerator::GenerateSinglePlaneMesh(glm::vec3(0, 0, 0), 1.0f));
	//model.SetMesh(Assets::MeshGenerator::GenerateDisconnectedPlaneMesh(glm::vec3(0, 0, 0), 1.0f, 10));
	//plane.SetMesh(Assets::MeshGenerator::GeneratePlaneMesh(glm::vec3(0, 0, 0), 0.5f, 100));
	plane.SetMesh(Assets::MeshGenerator::GenerateCubeMesh(glm::vec3(0, 0, 0), 1.0f));
	plane.ID = "Custom Mesh";
	myScene->AddRenderableObject(&plane);

	CustomObject cube = {};
	cube.SetMesh(Assets::MeshGenerator::GenerateCubeMesh(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f));
	cube.ID = "Cube";
	myScene->AddRenderableObject(&cube);

	/*
	CustomObject model = CustomObject();
	model.ID = "Sponza";
	model.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj";
	model.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master";
	model.Transformations.scaleHandler = 0.008f;
	myScene->AddRenderableObject(&model);
	CustomObject testObject = CustomObject();
	testObject.ID = "Backpack";
	testObject.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj";
	testObject.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack";
	testObject.FlipTexturesVertically = true;
	testObject.Transformations.translation.y = 5.714f;
	testObject.Transformations.translation.z = -0.095f;
	testObject.Transformations.rotation.y = 87.6f;
	testObject.Transformations.scaleHandler = 0.219f;
	myScene->AddRenderableObject(&testObject);
	*/

	Engine::Settings settings;
	settings.Title = "VulkanApplication.exe";
	settings.Width = 1600;
	settings.Height = 900;
	settings.uiEnabled = true;

	std::unique_ptr<Engine::Application> app = std::make_unique<Engine::Application>(settings);

	app->SetActiveScene(myScene.get());
	app->Init();

	try {
		app->Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	myScene.reset();
	app.reset();

	return EXIT_SUCCESS;
}
