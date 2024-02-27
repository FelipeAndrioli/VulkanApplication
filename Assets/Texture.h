#pragma once

#include <memory>
#include <string>

namespace Engine {
	class LogicalDevice;
	class PhysicalDevice;
	class CommandPool;
	class Image;
}

namespace Assets {
	enum TextureType {
		AMBIENT				= 0,
		DIFFUSE				= 1,
		SPECULAR			= 2,
		SPECULAR_HIGHTLIGHT = 3,
		BUMP				= 4,
		DISPLACEMENT		= 5,
		ALPHA				= 6,
		REFLECTION			= 7,
		ROUGHNESS			= 8,
		METALLIC			= 9,
		SHEEN				= 10,
		EMISSIVE			= 11,
		NORMAL				= 12
	};

	struct TextureIndices {
		alignas(16) int Ambient = 0;
		alignas(16) int Diffuse = 0;
		alignas(16)	int Specular = 0;
		alignas(16) int Bump = 0;
		alignas(16) int Roughness = 0;
		alignas(16) int Metallic = 0;
		alignas(16) int Normal = 0;
	};

	struct Texture {
		std::string Name = "";
		TextureType Type;
		size_t Index = 0;
		std::unique_ptr<class Engine::Image> TextureImage;
	};
}
