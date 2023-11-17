#include "Scene.h"

namespace Assets {
	Scene::Scene() {
		std::cout << "Starting scene" << std::endl;
	}

	Scene::~Scene() {
		for (auto model : Models) {
			model->ResetResources();
		}

		for (Engine::ResourceSet* resourceSet : ResourceSets) {
			delete resourceSet;
		}
	}

	void Scene::AddModel(Model* model) {
		Models.push_back(model);

		for (size_t i = 0; i < Models.size(); i++) {
			if (Models.size() == 1) break;

			for (size_t j = i + 1; j < Models.size(); j++) {
				if (Models[i]->ResourceSetIndex < Models[j]->ResourceSetIndex) {
					Model* tempModel = Models[i];
					Models[i] = Models[j];
					Models[j] = tempModel;
				}
			}
		}
	}

	void Scene::AddResourceSetLayout(Engine::ResourceSetLayout* resourceSetLayout) {
		m_ResourceSetLayouts.push_back(resourceSetLayout);
	}

	void Scene::OnCreate() {
		for (auto model : Models) {
			model->OnCreate();
		}
	}

	void Scene::OnUIRender() {
		for (auto model : Models) {
			model->OnUIRender();
		}
	}

	void Scene::OnUpdate(float t) {
		for (auto model : Models) {
			model->OnUpdate(t);
		}
	}

	void Scene::SetupScene(Engine::LogicalDevice* logicalDevice, Engine::PhysicalDevice* physicalDevice, 
		Engine::CommandPool* commandPool, Engine::SwapChain* swapChain) {
	
		int maxIndex = 0;

		for (Engine::ResourceSetLayout* resourceSetLayout : m_ResourceSetLayouts) {
			if (resourceSetLayout->ResourceSetIndex > maxIndex) maxIndex = resourceSetLayout->ResourceSetIndex;
		}

		ResourceSets.resize(maxIndex + 1);

		for (Engine::ResourceSetLayout* resourceSetLayout : m_ResourceSetLayouts) {
			ResourceSets[resourceSetLayout->ResourceSetIndex] = new Engine::ResourceSet(resourceSetLayout, logicalDevice,
				physicalDevice, commandPool, swapChain, Models);
		}
		
		CreateRenderPassBeginInfo(swapChain);
	}

	void Scene::Resize(Engine::SwapChain* swapChain) {
		for (size_t i = 0; i < ResourceSets.size(); i++) {
			ResourceSets[i]->Resize();
		}

		for (VkRenderPassBeginInfo* beginInfo : RenderPassBeginInfo) {
			delete beginInfo;
		}

		RenderPassBeginInfo.clear();

		CreateRenderPassBeginInfo(swapChain);
	}

	void Scene::CreateRenderPassBeginInfo(Engine::SwapChain* swapChain) {
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		for (size_t i = 0; i < swapChain->GetSwapChainImages().size(); i++) {
			VkRenderPassBeginInfo* renderPassInfo = new VkRenderPassBeginInfo[ResourceSets.size() + 1];

			for (size_t j = 0; j < ResourceSets.size(); j++) {
				VkRenderPassBeginInfo newRenderPassInfo{};

				newRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				newRenderPassInfo.renderPass = ResourceSets[j]->GetGraphicsPipeline()->GetRenderPass().GetHandle();
				newRenderPassInfo.framebuffer = *ResourceSets[j]->GetFramebuffer(i);
				newRenderPassInfo.renderArea.offset = {0, 0};
				newRenderPassInfo.renderArea.extent = swapChain->GetSwapChainExtent();
				newRenderPassInfo.pNext = nullptr;

				newRenderPassInfo.clearValueCount = 1;
				newRenderPassInfo.pClearValues = &clearColor;

				renderPassInfo[j] = newRenderPassInfo;
			}

			RenderPassBeginInfo.push_back(renderPassInfo);
		}
	}
}
