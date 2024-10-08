#include "ModelLoader.h"

#include <unordered_map>
#include <cassert>
#include <limits>

#include <assimp/mesh.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "../Assets/Model.h"
#include "../Assets/Mesh.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/ConstantBuffers.h"

#include "./Helper.h"

#include "./TextureLoader.h"

constexpr auto UNEXISTENT = -1;

int GetTextureIndex(std::vector<Texture>& loadedTextures, std::string textureName) {
	if (loadedTextures.size() == 0) 
		return UNEXISTENT;

	for (int i = 0; i < loadedTextures.size(); i++) {
		if (loadedTextures[i].Name == textureName) {
			return i;
		}
	}

	return UNEXISTENT;
}

int GetMaterialIndex(std::vector<Material>& sceneMaterials, std::string materialName) {
	if (sceneMaterials.size() == 0)
		return UNEXISTENT;

	for (int i = 0; i < sceneMaterials.size(); i++) {
		if (sceneMaterials[i].Name == materialName)
			return i;
	}

	return UNEXISTENT;
}

Assets::Mesh ProcessMesh(const aiMesh* mesh, const aiScene* scene) {
	std::vector<Assets::Vertex> vertices;
	std::vector<uint32_t> indices;
	Assets::Mesh newMesh = {};
	std::unordered_map<Assets::Vertex, uint32_t> uniqueVertices = {};

	for (size_t i = 0; i < mesh->mNumFaces; i++) {

		const aiFace face = mesh->mFaces[i];

		for (size_t j = 0; j < face.mNumIndices; j++) {
			Assets::Vertex vertex = {};
			vertex.pos = {
				mesh->mVertices[face.mIndices[j]].x,
				mesh->mVertices[face.mIndices[j]].y,
				mesh->mVertices[face.mIndices[j]].z
			};

			if (mesh->mTextureCoords[0]) {
				vertex.texCoord = {
					mesh->mTextureCoords[0][face.mIndices[j]].x,
					mesh->mTextureCoords[0][face.mIndices[j]].y
				};
			}

			if (mesh->HasNormals()) {
				vertex.normal = {
					mesh->mNormals[face.mIndices[j]].x,
					mesh->mNormals[face.mIndices[j]].y,
					mesh->mNormals[face.mIndices[j]].z
				};
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(newMesh.Vertices.size());
				newMesh.Vertices.push_back(vertex);
			}

			newMesh.Indices.push_back(uniqueVertices[vertex]);
		}
	}

	newMesh.MaterialName = scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
	
	return newMesh;
}

void ProcessNode(Assets::Model& model, const aiNode* node, const aiScene* scene) {
	for (size_t i = 0; i < node->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		model.Meshes.push_back(ProcessMesh(mesh, scene));
	}

	for (size_t i = 0; i < node->mNumChildren; i++) {
		ProcessNode(model, node->mChildren[i], scene);
	}
}

void LinkMeshesToMaterials(std::vector<Assets::Mesh>& meshes, std::vector<Material>& sceneMaterials) {
	for (auto& mesh : meshes) {
		if (GetMaterialIndex(sceneMaterials, mesh.MaterialName) == UNEXISTENT) {
			sceneMaterials.push_back(mesh.CustomMeshMaterial);
			mesh.MaterialName = "Default";
		} 

		mesh.MaterialIndex = GetMaterialIndex(sceneMaterials, mesh.MaterialName);
	}
}

void ValidateAndInsertTexture(
		std::vector<Texture>& loadedTextures,
		Texture::TextureType textureType,
		std::string textureName,
		std::string basePath,
		bool flipTexturesVertically,
		bool generateMipMaps
	) {

	int textureIndex = GetTextureIndex(loadedTextures, textureName);

	if (textureIndex != -1)
		return;

	loadedTextures.push_back(
		TextureLoader::LoadTexture(
			(basePath + textureName).c_str(),
			textureType,
			flipTexturesVertically,
			generateMipMaps
		)
	);

	loadedTextures[loadedTextures.size() - 1].Name = textureName;
}

void LoadTextureToMaterial(
	std::vector<Material>& sceneMaterials,
	std::vector<Texture>& loadedTextures,
	Texture::TextureType textureType,
	std::string textureName,
	std::string materialName
) {
	int materialIndex = GetMaterialIndex(sceneMaterials, materialName);
	switch (textureType) {
	case Texture::TextureType::AMBIENT:
		sceneMaterials[materialIndex].MaterialData.AmbientTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
		break;
	case Texture::TextureType::DIFFUSE:
		sceneMaterials[materialIndex].MaterialData.DiffuseTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
		break;
	case Texture::TextureType::SPECULAR:
		sceneMaterials[materialIndex].MaterialData.SpecularTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
		break;
	case Texture::TextureType::BUMP:
		sceneMaterials[materialIndex].MaterialData.BumpTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
		break;
	case Texture::TextureType::ROUGHNESS:
		sceneMaterials[materialIndex].MaterialData.RoughnessTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
		break;
	case Texture::TextureType::METALLIC:
		sceneMaterials[materialIndex].MaterialData.MetallicTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
		break;
	case Texture::TextureType::NORMAL:
		sceneMaterials[materialIndex].MaterialData.NormalTextureIndex = static_cast<uint32_t>(GetTextureIndex(loadedTextures, textureName));
		break;
	default:
		break;
	};
}

void ProcessTexture(
	std::vector<Material>& sceneMaterials,
	std::vector<Texture>& loadedTextures,
	Texture::TextureType textureType,
	std::string textureName,
	std::string basePath,
	std::string materialName,
	bool flipTexturesVertically,
	bool generateMipMaps
) {
	std::string texName = textureName;
	std::string path = basePath;

	if (textureName == "" || !Helper::file_exists(basePath + textureName)) {
		std::cout << "File: " << basePath + textureName << " doesn't exists! Loading custom texture!" << '\n';
		texName = "error_texture.jpg";
		path = "./Textures/";
	}

	ValidateAndInsertTexture(
		loadedTextures, 
		textureType, 
		texName, 
		path, 
		flipTexturesVertically,
		generateMipMaps
	);
	LoadTextureToMaterial(sceneMaterials, loadedTextures, textureType, texName, materialName);
}

void LoadTextures(
	Assets::Model& model, 
	aiMaterial* material, 
	aiTextureType textureType, 
	Texture::TextureType customTextureType,
	std::vector<Material>& sceneMaterials,
	std::vector<Texture>& loadedTextures
) {
	
	if (material == nullptr)
		return;

	for (uint32_t i = 0; i < material->GetTextureCount(textureType); i++) {
		aiString util;

		material->GetTexture(textureType, i, &util);

		ProcessTexture(
			sceneMaterials, 
			loadedTextures, 
			customTextureType, 
			util.C_Str(), 
			model.MaterialPath, 
			material->GetName().C_Str(), 
			//model.FlipTexturesVertically, 
			false,
			model.GenerateMipMaps
		);
	}
}

static void ProcessMaterials(
	Assets::Model& model,
	const aiScene* scene, 
	std::vector<Material>& sceneMaterials,
	std::vector<Texture>& loadedTextures) {

	int material_count = 0;

	for (size_t i = 0; i < scene->mNumMaterials; i++) {
		aiMaterial* material = scene->mMaterials[i];

		aiString materialName;
		aiColor4D diffuseColor;
		aiColor4D specularColor;
		aiColor4D ambientColor;
		aiColor4D emissiveColor;
		aiColor4D transparentColor;
		float opacity;
		float shininess;
		float shininessStrength;

		material->Get(AI_MATKEY_NAME, materialName);
		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
		material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
		material->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
		material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
		material->Get(AI_MATKEY_COLOR_TRANSPARENT, transparentColor);
		material->Get(AI_MATKEY_OPACITY, opacity);
		material->Get(AI_MATKEY_SHININESS, shininess);
		material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);

		if (GetMaterialIndex(sceneMaterials, materialName.C_Str()) == UNEXISTENT) {
			Material newMaterial = {};
			newMaterial.Name = materialName.C_Str();
			newMaterial.MaterialData.Ambient = glm::vec4(ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);
			newMaterial.MaterialData.Diffuse = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
			newMaterial.MaterialData.Specular = glm::vec4(specularColor.r, specularColor.g, specularColor.b, specularColor.a);
			newMaterial.MaterialData.Emission = glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, emissiveColor.a);
			newMaterial.MaterialData.Transparency = glm::vec4(transparentColor.r, transparentColor.g, transparentColor.b, transparentColor.a);
			newMaterial.MaterialData.Opacity = opacity;
			newMaterial.MaterialData.Shininess = shininess;
			newMaterial.MaterialData.ShininessStrength = shininessStrength;

			std::cout << "Material " << materialName.C_Str() << " shininess: " << shininess << '\n';
			sceneMaterials.push_back(newMaterial);

			material_count++;
		}

		LoadTextures(model, material, aiTextureType_AMBIENT, Texture::TextureType::AMBIENT, sceneMaterials, loadedTextures);
		LoadTextures(model, material, aiTextureType_DIFFUSE, Texture::TextureType::DIFFUSE, sceneMaterials, loadedTextures);
		LoadTextures(model, material, aiTextureType_SPECULAR, Texture::TextureType::SPECULAR, sceneMaterials, loadedTextures);
		LoadTextures(model, material, aiTextureType_NORMALS, Texture::TextureType::NORMAL, sceneMaterials, loadedTextures);
		LoadTextures(model, material, aiTextureType_HEIGHT, Texture::TextureType::BUMP, sceneMaterials, loadedTextures);
	}

	std::cout << material_count << " materials loaded" << '\n';
}

void ModelLoader::CompileMesh(Assets::Model& model, std::vector<Material>& materials) {

	std::vector<Assets::Vertex> vertices;
	std::vector<uint32_t> indices;

	float min_x = std::numeric_limits<float>::max();
	float max_x = std::numeric_limits<float>::min();

	float min_y = std::numeric_limits<float>::max();
	float max_y = std::numeric_limits<float>::min();
	
	float min_z = std::numeric_limits<float>::max();
	float max_z = std::numeric_limits<float>::min();


	for (auto& mesh : model.Meshes) {
		if (GetMaterialIndex(materials, mesh.MaterialName) == UNEXISTENT) {
			materials.push_back(mesh.CustomMeshMaterial);
			mesh.MaterialName = "Default";
		}

		mesh.MaterialIndex = GetMaterialIndex(materials, mesh.MaterialName);

		mesh.IndexOffset = indices.size();
		mesh.VertexOffset = vertices.size();

		indices.insert(indices.end(), mesh.Indices.begin(), mesh.Indices.end());
		vertices.insert(vertices.end(), mesh.Vertices.begin(), mesh.Vertices.end());

		for (const auto& vertex: mesh.Vertices) {
			if (vertex.pos.x > max_x)
				max_x = vertex.pos.x;
			if (vertex.pos.x < min_x)
				min_x = vertex.pos.x;
			if (vertex.pos.y > max_y)
				max_y = vertex.pos.y;
			if (vertex.pos.y < min_y)
				min_y = vertex.pos.y;
			if (vertex.pos.z > max_z)
				max_z = vertex.pos.z;
			if (vertex.pos.z < min_z)
				min_z = vertex.pos.z;	
		}
	}

	model.PivotVector = glm::vec3((max_x + min_x) / 2, (max_y + min_y) / 2, (max_z + min_z) / 2);

	model.TotalVertices = vertices.size();
	model.TotalIndices = indices.size();

	GraphicsDevice* gfxDevice = GetDevice();

	BufferDescription desc = {};
	desc.BufferSize = sizeof(Assets::Vertex) * vertices.size() + sizeof(uint32_t) * indices.size();
	desc.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	desc.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	gfxDevice->CreateBuffer(desc, model.DataBuffer, desc.BufferSize);

	gfxDevice->WriteBuffer(model.DataBuffer, indices.data(), sizeof(uint32_t) * indices.size(), 0);
	gfxDevice->WriteBuffer(model.DataBuffer, vertices.data(), sizeof(Assets::Vertex) * vertices.size(), sizeof(uint32_t) * indices.size());

	InputLayout modelInputLayout = {
		.bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT }
		}
	};

	model.ModelBuffer = gfxDevice->CreateBuffer(sizeof(ModelConstants));
	gfxDevice->CreateDescriptorSetLayout(model.ModelDescriptorSetLayout, modelInputLayout.bindings);
	gfxDevice->CreateDescriptorSet(model.ModelDescriptorSetLayout, model.ModelDescriptorSet);
	gfxDevice->WriteDescriptor(modelInputLayout.bindings[0], model.ModelDescriptorSet, model.ModelBuffer);
}

std::shared_ptr<Assets::Model> ModelLoader::LoadModel(const std::string& path, std::vector<Material>& materials, std::vector<Graphics::Texture>& textures) {
	
	std::shared_ptr<Assets::Model> model = std::make_shared<Assets::Model>();
	model->ModelPath = path.c_str();
	model->MaterialPath = Helper::get_directory(path);
	model->Name = Helper::get_filename(path);

	const aiScene* scene = aiImportFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);

	assert(scene && scene->HasMeshes());

	ProcessNode(*model.get(), scene->mRootNode, scene);
	ProcessMaterials(*model.get(), scene, materials, textures);
	CompileMesh(*model.get(), materials);

	return model;
}
