/*
	TODO list to when the setup is finihed

	- Next steps
		- Start to design Scenes
	- Compile the shaders inside the code instead of using glslc.exe
	- Future Challenges
*/

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

#include "Application.h"
#include "../Assets/Scene.h"
#include "../Assets/Model.h"

class MyScene : public Assets::Scene {
public:
	~MyScene() {
		DeleteModels();
	}

	void OnCreate() {

		/*
			{ {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },
			{ {0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },
			{ {0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} },
			{ {-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f} }
		*/
		Assets::Model* q1 = new Assets::Model();

		std::vector<Assets::Vertex> v = { 
			{ {-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f} },
			{ {0.0f, -1.0f}, {0.0f, 1.0f, 0.0f} },
			{ {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
			{ {-1.0f, 0.0f}, {1.0f, 1.0f, 1.0f} }
		};

		std::vector<uint16_t> i = {
			0, 1, 2, 2, 3, 0
		};

		q1->SetVertices(v);
		q1->SetIndices(i);
		q1->m_ID = "q1";
		AddModel(q1);

		Assets::Model* q2 = new Assets::Model();

		std::vector<Assets::Vertex> v2 = { 
			{ {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
			{ {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
			{ {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
			{ {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f} }
		};

		std::vector<uint16_t> i2 = {
			0, 1, 2, 2, 3, 0
		};

		q2->SetVertices(v2);
		q2->SetIndices(i2);

		q2->m_ID = "q2";
		AddModel(q2); 
	}

	void OnUIRender() {
		for (auto model : GetSceneModels()) {
			std::string modelId = model->m_ID;

			std::string t = "Transformations " + modelId;

			if (ImGui::TreeNode(t.c_str())) {
				std::string t_label_x = "Translation x " + modelId;
				std::string t_label_y = "Translation y " + modelId;
				std::string t_label_z = "Translation z " + modelId;

				ImGui::SliderFloat(t_label_x.c_str(), &model->m_Transform.translation.x, 0.0f, 1.0f);
				ImGui::SliderFloat(t_label_y.c_str(), &model->m_Transform.translation.y, 0.0f, 1.0f);
				ImGui::SliderFloat(t_label_z.c_str(), &model->m_Transform.translation.z, 0.0f, 1.0f);
				ImGui::TreePop();
			}
		}
	}

	void OnUpdate(float time) {

	}

	void OnDestroy() {

	}
};

class RenderLayout {
	struct GraphicsPipeline {
		/* Resources required from graphics pipeline
			- binding description
			- attribute description
			- the graphics pipeline always need a vertex input state?
			- topology
			- polygon mode
				- fill
				- line
				- point
				- rectangle fill
			- cull mode
				- cull back
				- cull front
				- cull front and back
			- front face
				- counter clockwise
				- clockwise
			- multisampling

			- resources inside the graphics pipeline to make dynamic
				- descriptor set layout
				- descriptor pool
				- graphics pipeline layout
				- render pass
		*/
	};
};

int main() {
	Engine::WindowSettings windowSettings;
	windowSettings.Title = "VulkanApplication.exe";
	windowSettings.Width = 800;
	windowSettings.Height = 600;

	Engine::UserSettings userSettings;
	userSettings.rayTraced = true;
	
	Engine::Application* app = new Engine::Application(windowSettings, userSettings);

	MyScene* myScene = new MyScene();
	myScene->OnCreate();

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