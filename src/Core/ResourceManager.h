#pragma once

#include <vector>

#include "./Graphics.h"
#include "./GraphicsDevice.h"

#include "../Assets/Material.h"

class ResourceManager {
public:
	static ResourceManager* Get();
	void Destroy();

	const int GetLastTextureIndex()		{ return static_cast<int>(m_Textures.size() - 1);	}
	const int GetTotalTextures()		{ return static_cast<int>(m_Textures.size());		}

	const int GetLastMaterialIndex()	{ return static_cast<int>(m_Materials.size() - 1);	}
	const int GetTotalMaterials()		{ return static_cast<int>(m_Materials.size());		}

	void AddMaterial(Material material);
	void AddTexture(Graphics::Texture texture);

	int GetMaterialIndex(const std::string& materialName);
	int GetTextureIndex(const std::string& textureName);

	Material& GetMaterial(int idx) { return m_Materials[idx]; }

	std::vector<Graphics::Texture>& GetTextures() { return m_Textures; }
	Graphics::Buffer& GetMaterialBuffer();

private:
	ResourceManager();
	ResourceManager(ResourceManager& other) = delete;
	void operator=(const ResourceManager&) = delete;

	std::vector<Material> m_Materials;
	std::vector<Graphics::Texture> m_Textures;

	Graphics::Buffer m_MaterialUBO;			// double buffer?

	static ResourceManager* m_Instance;
};