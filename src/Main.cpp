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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Application.h"
#include "UI.h"

#include "../Assets/Camera.h"
#include "../Assets/Scene.h"
#include "../Assets/Object.h"
#include "../Assets/Shader.h"
#include "../Assets/Pipeline.h"

class MyCubeOne : public Assets::Object {
public:
	bool Rotate = false;

	void OnCreate() {

		std::vector<Assets::Vertex> v = {
			{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},		// top front right		0 - OK
			{{ 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},		// top back right		1 - OK
			{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},		// top front left		2 - OK
			{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},		// top back left		3 - OK
			{{ -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},		// bottom front left	4 - OK
			{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},	// bottom back left		5 - OK
			{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},		// bottom front right	6 - OK
			{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }}		// bottom back right	7 - OK
		};

		std::vector<uint32_t> i = {
			0, 2, 6, 2, 4, 6,		// front face
			0, 6, 7, 0, 1, 7,		// right face
			3, 4, 2, 3, 5, 4,		// left face
			3, 5, 7, 3, 7, 1,		// bottom face
			2, 3, 0, 3, 0, 1,		// top face
			5, 4, 6, 5, 6, 7		// bottom face
		};

		Transformations.translation.z = -4.0f;

		//this->Meshes.push_back({ v, i });
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

			ImGui::Checkbox("Rotate", &Rotate);
			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		if (Rotate) {
			Transformations.rotation.x += t * 0.05f;
		}
	}
};

class MyCubeTwo : public Assets::Object {
public:
	void OnCreate() {

		std::vector<Assets::Vertex> v = {
			{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},		// top front right		0 - OK
			{{ 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},		// top back right		1 - OK
			{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},		// top front left		2 - OK
			{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},		// top back left		3 - OK
			{{ -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},		// bottom front left	4 - OK
			{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},	// bottom back left		5 - OK
			{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},		// bottom front right	6 - OK
			{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }}		// bottom back right	7 - OK
		};

		std::vector<uint32_t> i = {
			0, 2, 6, 2, 4, 6,		// front face
			0, 6, 7, 0, 1, 7,		// right face
			3, 4, 2, 3, 5, 4,		// left face
			3, 5, 7, 3, 7, 1,		// bottom face
			2, 3, 0, 3, 0, 1,		// top face
			5, 4, 6, 5, 6, 7		// bottom face
		};

		Transformations.translation.x = 2.0f;
		Transformations.translation.z = -4.0f;

		//this->Meshes.push_back({ v, i });
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
			
			//ImGui::ColorEdit3("Color", glm::value_ptr(ubo->color));

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		Transformations.rotation.y += t * 0.05f;
	}
};

class MyCubeThree : public Assets::Object {
public:

	void OnCreate() {

		std::vector<Assets::Vertex> v = {
			{{ 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},		// top front right		0 - OK
			{{ 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},		// top back right		1 - OK
			{{ -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},		// top front left		2 - OK
			{{ -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},		// top back left		3 - OK
			{{ -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }},		// bottom front left	4 - OK
			{{ -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},	// bottom back left		5 - OK
			{{ 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }},		// bottom front right	6 - OK
			{{ 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }}		// bottom back right	7 - OK
		};

		std::vector<uint32_t> i = {
			0, 2, 6, 2, 4, 6,		// front face
			0, 6, 7, 0, 1, 7,		// right face
			3, 4, 2, 3, 5, 4,		// left face
			3, 5, 7, 3, 7, 1,		// bottom face
			2, 3, 0, 3, 0, 1,		// top face
			5, 4, 6, 5, 6, 7		// bottom face
		};

		Transformations.translation.x = -2.0f;
		Transformations.translation.z = -4.0f;

		//this->Meshes.push_back({ v, i });
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

			ImGui::TreePop();
		}
	}

	void OnUpdate(float t) {
		Transformations.rotation.z += t * 0.05f;
	}
};

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

	/*
	std::unique_ptr<Engine::Material> rainbowMaterial = std::make_unique<Engine::Material>();
	rainbowMaterial->Layout.ID = "RainbowMaterial";
	rainbowMaterial->Layout.VertexShaderPath = "./Assets/Shaders/vert.spv";
	rainbowMaterial->Layout.FragmentShaderPath = "./Assets/Shaders/frag.spv";

	std::unique_ptr<Engine::Material> colorMaterial = std::make_unique<Engine::Material>();
	colorMaterial->Layout.ID = "ColorMaterial";
	colorMaterial->Layout.VertexShaderPath = "./Assets/Shaders/shader_test_vert.spv";
	colorMaterial->Layout.FragmentShaderPath = "./Assets/Shaders/shader_test_frag.spv";

	std::unique_ptr<Engine::Material> texturedMaterial = std::make_unique<Engine::Material>();
	texturedMaterial->Layout.ID = "TexturedMaterial";
	texturedMaterial->Layout.VertexShaderPath = "./Assets/Shaders/textured_vert.spv";
	texturedMaterial->Layout.FragmentShaderPath = "./Assets/Shaders/textured_frag.spv";
	texturedMaterial->Layout.TexturePath = "./Assets/Textures/statue_test.jpg";

	std::unique_ptr<Assets::Scene> myScene = std::make_unique<Assets::Scene>();

	myScene->AddMaterial(rainbowMaterial.get());
	myScene->AddMaterial(colorMaterial.get());
	myScene->AddMaterial(texturedMaterial.get());

	auto q1 = MyCubeOne();
	q1.Rotate = true;
	q1.Material = rainbowMaterial.get();
	q1.SceneCamera = myScene->MainCamera;
	auto q2 = MyCubeTwo();
	q2.SceneCamera = myScene->MainCamera;
	q2.Material = colorMaterial.get();
	auto q3 = MyCubeThree();
	q3.SceneCamera = myScene->MainCamera;
	auto q4 = MyCubeOne();
	q4.Transformations.translation.y = 2.0f;
	q4.Transformations.rotation.x = 200.0f;
	q4.Material = texturedMaterial.get();
	q4.SceneCamera = myScene->MainCamera;

	myScene->AddObject(&q1);
	myScene->AddObject(&q2);
	myScene->AddObject(&q3);
	myScene->AddObject(&q4);
	*/

	/*
	std::unique_ptr<Engine::Material> texturedMaterial = std::make_unique<Engine::Material>();
	texturedMaterial->Layout.ID = "TexturedMaterial";
	texturedMaterial->Layout.VertexShaderPath = "./Assets/Shaders/textured_vert.spv";
	texturedMaterial->Layout.FragmentShaderPath = "./Assets/Shaders/textured_frag.spv";
	texturedMaterial->Layout.TexturePath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/diffuse.jpg";
	*/

	std::unique_ptr<Assets::Scene> myScene = std::make_unique<Assets::Scene>();

	Assets::VertexShader defaultVertexShader = Assets::VertexShader("Default Vertex Shader", "./Assets/Shaders/vert.spv");
	Assets::FragmentShader defaultFragmentShader = Assets::FragmentShader("Default Fragment Shader", "./Assets/Shaders/frag.spv"); 
	Assets::GraphicsPipeline defaultGraphicsPipeline = Assets::GraphicsPipeline("defaultPipeline", defaultVertexShader, defaultFragmentShader);
	myScene->AddGraphicsPipeline(defaultGraphicsPipeline);

	Assets::VertexShader texturedVertexShader = Assets::VertexShader("Textured Vertex Shader", "./Assets/Shaders/textured_vert.spv");
	Assets::FragmentShader texturedFragmentShader = Assets::FragmentShader("Textured Fragment Shader", "./Assets/Shaders/textured_frag.spv");
	Assets::GraphicsPipeline texturedGraphicsPipeline = Assets::GraphicsPipeline("texturedPipeline", texturedVertexShader, texturedFragmentShader);
	myScene->AddGraphicsPipeline(texturedGraphicsPipeline);

	Assets::VertexShader wireframeVertexShader = Assets::VertexShader("Default Vertex Shader", "./Assets/Shaders/textured_vert.spv");
	Assets::FragmentShader wireframeFragShader = Assets::FragmentShader("Wireframe Fragment Shader", "./Assets/Shaders/textured_frag.spv");
	wireframeFragShader.PolygonMode = Assets::FragmentShader::Polygon::LINE;
	Assets::GraphicsPipeline wireFramePipeline = Assets::GraphicsPipeline("wireframePipeline", wireframeVertexShader, wireframeFragShader);
	myScene->AddGraphicsPipeline(wireFramePipeline);

	CustomObject model = CustomObject();
	model.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj";
	model.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master";
	model.PipelineName = texturedGraphicsPipeline.Name;
	model.Transformations.scaleHandler = 0.008f;
	myScene->AddRenderableObject(&model);

	/*
	CustomObject testObject = CustomObject();
	testObject.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj";
	testObject.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack";
	testObject.FlipTexturesVertically = true;
	testObject.PipelineName = texturedGraphicsPipeline.Name;

	testObject.Transformations.translation.z = -2.0f;

	myScene->AddRenderableObject(&testObject);

	CustomObject testObject2 = CustomObject();
	testObject2.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj";
	testObject2.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack";
	testObject2.FlipTexturesVertically = true;
	testObject2.PipelineName = wireFramePipeline.Name;
	testObject2.Transformations.translation.x = 5.0f;
	testObject2.Transformations.translation.z = -2.0f;
	
	myScene->AddRenderableObject(&testObject2);

	CustomObject sword = CustomObject();
	sword.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/sword/sword.obj";
	sword.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/sword";
	sword.PipelineName = defaultGraphicsPipeline.Name;
	sword.Transformations.translation.x = -5.0f;
	sword.Transformations.translation.z = -2.0f;
	myScene->AddRenderableObject(&sword);
	
	CustomObject model = CustomObject();
	model.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/square/model.obj";
	model.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/square/";
	//model.PipelineName = defaultGraphicsPipeline.Name;
	model.PipelineName = texturedGraphicsPipeline.Name;
	myScene->AddRenderableObject(&model);
	*/

	Engine::Settings settings;
	settings.Title = "VulkanApplication.exe";
	settings.Width = 1600;
	settings.Height = 900;

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

	//rainbowMaterial.reset();
	//colorMaterial.reset();
	//texturedMaterial.reset();
	
	myScene.reset();
	app.reset();

	return EXIT_SUCCESS;
}
