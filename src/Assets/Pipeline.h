#pragma once

#include "Shader.h"

namespace Assets {
	class GraphicsPipeline {
	public:
		GraphicsPipeline() {};
		GraphicsPipeline(
			std::string pipelineName, 
			VertexShader vertexShader, 
			FragmentShader fragmentShader) : Name(pipelineName), m_VertexShader(vertexShader), m_FragmentShader(fragmentShader) {}
		~GraphicsPipeline() {};

	public:
		std::string Name = "";
		VertexShader m_VertexShader;
		FragmentShader m_FragmentShader;
	};

	class ComputePipeline {
	public:
		ComputePipeline() {};
		ComputePipeline(std::string pipelineName, ComputeShader computeShader) : Name(pipelineName), m_ComputeShader(computeShader) {};
		~ComputePipeline() {};

	public:
		std::string Name = "";
		ComputeShader m_ComputeShader;
	};
}
