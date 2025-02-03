#include "ResourceManager.h"

#include "../Assets/Material.h"

#define TEXTURES_LIMIT 256
#define MATERIALS_LIMIT 50 

ResourceManager* ResourceManager::m_Instance = nullptr;

ResourceManager::ResourceManager() {
	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	m_MaterialUBO = gfxDevice->CreateBuffer(sizeof(MaterialData) * MATERIALS_LIMIT);

	m_Materials.push_back({});

	gfxDevice->WriteSubBuffer(m_MaterialUBO, &m_Materials[0].MaterialData, sizeof(MaterialData));
}

ResourceManager* ResourceManager::Get() {
	if (m_Instance == nullptr) {
		m_Instance = new ResourceManager();
	}

	return m_Instance;
}

void ResourceManager::Destroy() {
	if (m_Instance == nullptr)
		return;

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();

	for (auto texture : m_Textures) {
		gfxDevice->DestroyImage(texture);
	}

	m_Textures.clear();
	m_Materials.clear();

	delete m_Instance;
}

Graphics::Buffer& ResourceManager::GetMaterialBuffer() {
	return m_MaterialUBO;
}

void ResourceManager::AddTexture(Graphics::Texture texture) {
	// TODO: Use hash code instead of Name

	if (m_Textures.size() < TEXTURES_LIMIT) {
		m_Textures.push_back(texture);
	}
	else {
		std::cout << "Textures out of space!" << '\n';
		return;
	}
}

void ResourceManager::AddMaterial(Material material) {

	// TODO: Update buffer when inserting a new material
	// TODO: Use hash code instead of Name

	Graphics::GraphicsDevice* gfxDevice = Graphics::GetDevice();
	
	if (m_Materials.size() < MATERIALS_LIMIT) {
		m_Materials.push_back(material);

		gfxDevice->WriteSubBuffer(m_MaterialUBO, &material.MaterialData, sizeof(MaterialData));
	}
	else {
		std::cout << "Materials out of space!" << '\n';
		return;
	}
}

int ResourceManager::GetMaterialIndex(const std::string& materialName) {
	for (int i = 0; i < m_Materials.size(); i++) {
		if (m_Materials[i].Name == materialName)
			return i;
	}

	return -1;
}

int ResourceManager::GetTextureIndex(const std::string& textureName) {
	for (int i = 0; i < m_Textures.size(); i++) {
		if (m_Textures[i].Name == textureName)
			return i;
	}

	return -1;
}
