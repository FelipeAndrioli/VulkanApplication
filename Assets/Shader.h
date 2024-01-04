#pragma once

#include <iostream>
#include <memory>
#include <string>

namespace Assets {
	class Shader {
	public:
		enum Topology {
			POINT_LIST					= 0,
			LINE_LIST					= 1,
			LINE_STRIP					= 2,
			TRIANGLE_LIST				= 3,
			TRIANGLE_STRIP				= 4,
			TRIANGLE_FAN				= 5,
			LINE_LIST_W_ADJACENCY		= 6,
			LINE_STRIP_W_ADJACENCY		= 7,
			TRIANGLE_LIST_W_ADJACENCY	= 8,
			TRIANGLE_STRIP_W_ADJACENCY	= 9,
			PATCH_LIST					= 10
		};

		enum Polygon {
			FILL			= 0,
			LINE			= 1,
			POINT			= 2,
			RECTANGLE_FILL	= 3
		};

		enum Culling {
			CULL_BACK			= 0,
			CULL_FRONT			= 1,
			CULL_FRONT_AND_BACK = 2
		};

		enum FrontFace {
			COUNTER_CLOCKWISE	= 0,
			CLOCKWISE			= 1
		};

		enum Type {
			VERTEX		= 0,
			FRAGMENT	= 1,
			COMPUTE		= 2
		};

		Shader();
		~Shader();
	
	public:
		std::string Path = "";
		std::string Name = "";
		
		Type ShaderType;
		Topology TopologyMode = TRIANGLE_LIST;
		Polygon PolygonMode = FILL;
		Culling CullingMode = CULL_BACK;
		FrontFace FrontFaceMode = COUNTER_CLOCKWISE;

		float LineWidth = 1.0f;
	};
}
