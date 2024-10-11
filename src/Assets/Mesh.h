#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vector>
#include <array>

#include <glm.hpp>
#include <gtx/hash.hpp>

#include "../Core/VulkanHeader.h"

#include "Material.h"

namespace Assets {
	struct Vertex {

		glm::vec3 pos = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 normal = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec2 texCoord = glm::vec2(0.0f, 0.0f);

		bool operator==(const Vertex& other) const {
			return (pos == other.pos) && (normal == other.normal) && (color == other.color) && (texCoord == other.texCoord);
		}

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, normal);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, color);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}
	};
	
	struct Mesh {
		std::string MaterialName = "";

		uint16_t PSOFlags;

		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;

		size_t MaterialIndex = 0;
		size_t IndexOffset = 0;
		size_t VertexOffset = 0;

		glm::vec3 PivotVector = glm::vec3(1.0f);
	};	
}

namespace PSOFlags {
	enum : uint16_t {
		tOpaque = 0x001,
		tTransparent = 0x002,
		tTwoSided = 0x004,
		tOutline = 0x008,
		tWireframe = 0x010
	};
}

namespace std {
    template<> struct hash<Assets::Vertex> {
        size_t operator()(Assets::Vertex const& vertex) const {
            return hash<glm::vec3>()(vertex.pos) ^ hash<glm::vec3>()(vertex.normal) ^ hash<glm::vec3>()(vertex.color) ^ hash<glm::vec2>()(vertex.texCoord);
        }
    };
}

