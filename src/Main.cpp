#include <iostream>
#include <memory>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "Application.h"

#include "./Assets/Camera.h"
#include "./Assets/Scene.h"
#include "./Assets/Object.h"
#include "./Assets/Shader.h"
#include "./Assets/Pipeline.h"
#include "./Assets/Material.h"
#include "./Assets/Utils/MeshGenerator.h"

class CustomObject : public Assets::Object {
public:

	using Object::Object;

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
	myScene->SetCameraPosition({ 5, 2, 0 }, -181, -7);

	/*
	CustomObject plane = CustomObject();
	//model.SetMesh(Assets::MeshGenerator::GenerateSinglePlaneMesh(glm::vec3(0, 0, 0), 1.0f));
	//model.SetMesh(Assets::MeshGenerator::GenerateDisconnectedPlaneMesh(glm::vec3(0, 0, 0), 1.0f, 10));
	plane.SetMesh(Assets::MeshGenerator::GeneratePlaneMesh(glm::vec3(0, 0, 0), 0.5f, 100));
	//plane.SetMesh(Assets::MeshGenerator::GenerateCubeMesh(glm::vec3(0, 0, 0), 1.0f));
	plane.ID = "Custom Mesh";
	myScene->AddRenderableObject(&plane);

	CustomObject cube = {};
	cube.SetMesh(Assets::MeshGenerator::GenerateCubeMesh(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f));
	cube.ID = "Cube";
	myScene->AddRenderableObject(&cube);
	*/

	CustomObject model = CustomObject();
	model.ID = "Sponza";
	model.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master/sponza.obj";
	model.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/Sponza-master";
	model.Transformations.scaleHandler = 0.008f;
	myScene->AddRenderableObject(&model);

	CustomObject backpack = CustomObject(glm::vec3(0.0f, 5.7f, -0.09f));
	backpack.ID = "Backpack";
	backpack.ModelPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack/backpack.obj";
	backpack.MaterialPath = "C:/Users/Felipe/Documents/current_projects/models/actual_models/backpack";
	backpack.FlipTexturesVertically = true;
	backpack.Transformations.rotation.y = 87.6f;
	backpack.Transformations.scaleHandler = 0.219f;
	myScene->AddRenderableObject(&backpack);

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
