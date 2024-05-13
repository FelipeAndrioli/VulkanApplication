#include "Shader.h"

namespace Assets {
	VertexShader::VertexShader() {

	}

	VertexShader::VertexShader(std::string shaderName) : Name(shaderName) {

	}

	VertexShader::VertexShader(std::string shaderName, std::string shaderPath) : Name(shaderName), Path(shaderPath) {

	}

	VertexShader::~VertexShader() {

	}

	FragmentShader::FragmentShader() {

	}

	FragmentShader::FragmentShader(std::string shaderName) : Name(shaderName) {

	}
	
	FragmentShader::FragmentShader(std::string shaderName, std::string shaderPath) : Name(shaderName), Path(shaderPath) {

	}

	FragmentShader::~FragmentShader() {

	}

	ComputeShader::ComputeShader() {

	}

	ComputeShader::ComputeShader(std::string shaderName) : Name(shaderName) {

	}

	ComputeShader::ComputeShader(std::string shaderName, std::string shaderPath) : Name(shaderName), Path(shaderPath) {

	}
	
	ComputeShader::~ComputeShader() {

	}

}