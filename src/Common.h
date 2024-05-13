#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <array>
#include <optional>

#include "Vulkan.h"
#include "CommandBuffer.h"
#include "./Assets/Mesh.h"

namespace Engine {
	const int MAX_FRAMES_IN_FLIGHT = 2;
	const int TEXTURE_PER_MATERIAL = 7;
	const uint32_t PARTICLE_COUNT = 8192;

	const std::vector<const char*> c_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t>graphicsFamily;
		std::optional<uint32_t>presentFamily;
		std::optional<uint32_t>graphicsAndComputeFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value() && graphicsAndComputeFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct ComputeUniformBufferObject {
		float deltaTime = 1.0f;
	};

	struct Particle {
		glm::vec2 position;
		glm::vec2 velocity;
		glm::vec4 color;

		static VkVertexInputBindingDescription getBindindDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Particle);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Particle, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Particle, color);

			return attributeDescriptions;
		}
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	static void check_vk_result(VkResult err) {
		if (err == 0) return;
		fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
		if (err < 0) throw std::runtime_error("Something went wrong");
	}
}
