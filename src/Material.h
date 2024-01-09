#pragma once

#include "Vulkan.h"
#include "MaterialLayout.h"
#include "GraphicsPipeline.h"

namespace Assets {
	struct Texture;
}

namespace Engine {
	class GraphicsPipeline;
	class Buffer;
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class SwapChain;
	class DepthBuffer;
	class Image;

	class Material {
	public:

		Material();
		~Material();
	
		void Create(LogicalDevice& logicalDevice,
			PhysicalDevice& physicalDevice,
			CommandPool& commandPool,
			const SwapChain& swapChain,
			const DepthBuffer& depthBuffer,
			const VkRenderPass& renderPass
		);

		GraphicsPipeline* GetGraphicsPipeline() { return m_GraphicsPipeline.get(); };

		void LoadTexture(
			PhysicalDevice& physicalDevice, 
			LogicalDevice& logicalDevice, 
			CommandPool& commandPool
		);

	public:
		std::string Name = "";

		glm::vec3 Ambient = glm::vec3(1.0f);
		glm::vec3 Diffuse = glm::vec3(1.0f);
		glm::vec3 Specular = glm::vec3(1.0f);
		glm::vec3 Transmittance = glm::vec3(1.0f);
		glm::vec3 Emission = glm::vec3(1.0f);

		float Shininess = 0.0f;
		float Ior = 0.0f;
		float Dissolve = 0.0f;
		float Roughness = 0.0f;
		float Metallic = 0.0f;
		float Sheen = 0.0f;
		float ClearcoatThickness = 0.0f;
		float ClearcoatRoughness = 0.0f;
		float Anisotropy = 0.0f;
		float AnisotropyRotation = 0.0f;
		//float Pad0 = 0.0f;
		
		int Pad2 = 0;
		int Illum = 0;
		
		//MaterialLayout Layout = MaterialLayout();
		std::unique_ptr<class Image> Texture;
		std::vector<Assets::Texture> Textures;
	private:
		std::unique_ptr<class GraphicsPipeline> m_GraphicsPipeline;
	};
}
