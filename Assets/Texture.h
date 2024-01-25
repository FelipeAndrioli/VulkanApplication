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

	struct Texture {
		std::string Path = "";
		TextureType Type;
		std::unique_ptr<class Engine::Image> TextureImage;
	};
}
