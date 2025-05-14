#pragma once

#include "../Graphics.h"
#include "../GraphicsDevice.h"
#include "../VulkanHeader.h"

#include "../../Input/Input.h"

class IRenderer {
public:
	virtual void StartUp	() = 0;
	virtual void CleanUp	() = 0;
	virtual void Update		(const float d, const float c, const InputSystem::Input& input) = 0;
	virtual void Render		(const VkCommandBuffer& commandBuffer) = 0;
	virtual void RenderUI	() = 0;
};
