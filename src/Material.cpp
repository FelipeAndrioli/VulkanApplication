#include "Material.h"

#include "Buffer.h"
#include "BufferHelper.h"
#include "PipelineLayout.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"
#include "SwapChain.h"
#include "DepthBuffer.h"
#include "Image.h"
#include "./Utils/TextureLoader.h"

#include "../../Assets/Texture.h"

namespace Engine {
	Material::Material() {

	}

	Material::~Material() {

		for (Assets::Texture& texture : Textures) {
			texture.TextureImage.reset();
		}

		Texture.reset();
		Textures.clear();
		
		m_GraphicsPipeline.reset();
	}

	void Material::Create(LogicalDevice& logicalDevice, 
		PhysicalDevice& physicalDevice, 
		CommandPool& commandPool, 
		const SwapChain& swapChain, 
		const DepthBuffer& depthBuffer, 
		const VkRenderPass& renderPass
	) {
		//m_GraphicsPipeline.reset(new class GraphicsPipeline(Layout, logicalDevice, swapChain, depthBuffer, renderPass));

		//if (Layout.TexturePath != nullptr) LoadTexture(physicalDevice, logicalDevice, commandPool);
	}

	void Material::LoadTexture(PhysicalDevice& physicalDevice, LogicalDevice& logicalDevice, CommandPool& commandPool) {
		/*
		Utils::TextureLoader::LoadTexture(
			Texture,
			Layout.TexturePath,
			logicalDevice,
			physicalDevice,
			commandPool
		);
		*/
	}
}