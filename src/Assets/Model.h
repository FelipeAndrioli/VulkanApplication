#pragma once

#include <gtc/matrix_transform.hpp>

#include "Mesh.h"

#include "../Graphics.h"

struct Transform {
	glm::vec3 translation;
	glm::vec3 rotation;

	float scaleHandler;
};

class Model {
public:

	~Model();

	virtual void OnCreate() {};
	virtual void OnUpdate(float t);
	virtual void OnUIRender();
	void Destroy();

	glm::mat4 GetModelMatrix();

public:
	std::string Name = "";

	std::vector<Assets::Mesh> Meshes;
	std::vector<Engine::Graphics::Texture> Textures;

	Transform Transformations = {};

	size_t TotalVertices = 0;
	size_t TotalIndices = 0;

	bool FlipTexturesVertically = false;
	bool GenerateMipMaps = true;
	bool Rotate = false;
	
	const char* ModelPath = nullptr;
	const char* MaterialPath = nullptr;
	
	Engine::Graphics::GPUBuffer DataBuffer = {};
private:
	Engine::Graphics::GPUBuffer m_MaterialBuffer = {};
};