#pragma once

#include <iostream>
#include <array>

#include "Vulkan.h"

#include "../Assets/Vertex.h"

namespace Engine {
	struct ComputePipelineLayout {
		const char* computeShaderPath = "";
		bool lastFrameAccess = false;
	};

	struct GraphicsPipelineLayout {
		enum Topology {
			POINT_LIST = 0,
			LINE_LIST = 1,
			LINE_STRIP = 2,
			TRIANGLE_LIST = 3,
			TRIANGLE_STRIP = 4,
			TRIANGLE_FAN = 5,
			LINE_LIST_W_ADJACENCY = 6,
			LINE_STRIP_W_ADJACENCY = 7,
			TRIANGLE_LIST_W_ADJACENCY = 8,
			TRIANGLE_STRIP_W_ADJACENCY = 9,
			PATCH_LIST = 10
		};

		enum PolygonMode {
			FILL = 0,
			LINE = 1,
			POINT = 2,
			RECTANGLE_FILL = 3
		};

		enum CullMode {
			CULL_BACK = 0,
			CULL_FRONT = 1,
			CULL_FRONT_AND_BACK = 2
		};

		enum FrontFace {
			COUNTER_CLOCKWISE = 0,
			CLOCKWISE = 1
		};

		enum RenderType {
			DEFAULT_RENDER = 0
		};

		VkVertexInputBindingDescription bindingDescription = Assets::Vertex::getBindingDescription();
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = Assets::Vertex::getAttributeDescriptions();

		Topology topology = TRIANGLE_LIST;
		PolygonMode polygonMode = FILL;
		CullMode cullMode = CULL_BACK;
		FrontFace frontFace = COUNTER_CLOCKWISE;

		float lineWidth = 1.0f;
		size_t maxDescriptorSets = 2;
		
		const char* vertexShaderPath = "";
		const char* fragmentShaderPath = "";

		RenderType renderType = DEFAULT_RENDER;
		ComputePipelineLayout* computePipelineLayout = nullptr;

		std::string resourceSetID;
	};
}
