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
#include "../Assets/Utils/MeshGenerator.h"

#include "../Core/Graphics.h"
#include "../Core/GraphicsDevice.h"
#include "../Core/ConstantBuffers.h"
#include "../Core/ResourceManager.h"

#include "./Helper.h"

#include "./TextureLoader.h"

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

static void LoadMaterialTextures(aiMaterial* assimpMaterial, Material& material, const std::string& materialPath) {
	ResourceManager* rm = ResourceManager::Get();

	// TODO: Turn these arrays into constants or enums

	const int numTextureTypes = 5;

	const std::array<aiTextureType, numTextureTypes> assimpTextureTypes = {
		aiTextureType_AMBIENT,
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_NORMALS,
		aiTextureType_HEIGHT
	};

	const std::array<Texture::TextureType, numTextureTypes> textureTypes = {
		Texture::TextureType::AMBIENT,
		Texture::TextureType::DIFFUSE,
		Texture::TextureType::SPECULAR,
		Texture::TextureType::NORMAL,
		Texture::TextureType::BUMP,
	};

	for (uint32_t tt = 0; tt < numTextureTypes; tt++) {
		for (uint32_t t = 0; t < assimpMaterial->GetTextureCount(assimpTextureTypes[tt]); t++) {
			aiString materialName;
			aiString util;

			assimpMaterial->Get(AI_MATKEY_NAME, materialName);
			assimpMaterial->GetTexture(assimpTextureTypes[tt], t, &util);
			
			std::string texturePath = (materialPath + util.C_Str()).c_str();
			std::string path = materialPath;

			if (util.C_Str()  == "" || !Helper::file_exists(texturePath)) {
				std::cout << "File: " << texturePath << " doesn't exists! Loading custom texture!" << '\n';
				texturePath = "error_texture.jpg";
				path = "./Textures/";
			}
			else {
			}

			rm->AddTexture(
				TextureLoader::LoadTexture(
					texturePath.c_str(),
					textureTypes[tt],
					false,
					true //model.GenerateMipMaps	
				)
			);

			int textureIndex = rm->GetTextureIndex(Helper::get_filename(texturePath));

			switch (textureTypes[tt]) {
			case Texture::TextureType::AMBIENT:
				material.MaterialData.AmbientTextureIndex = textureIndex;
				break;
			case Texture::TextureType::DIFFUSE:
				material.MaterialData.DiffuseTextureIndex = textureIndex;
				break;
			case Texture::TextureType::SPECULAR:
				material.MaterialData.SpecularTextureIndex = textureIndex;
				break;
			case Texture::TextureType::BUMP:
				material.MaterialData.BumpTextureIndex = textureIndex;
				break;
			case Texture::TextureType::ROUGHNESS:
				material.MaterialData.RoughnessTextureIndex = textureIndex;
				break;
			case Texture::TextureType::METALLIC:
				material.MaterialData.MetallicTextureIndex = textureIndex;
				break;
			case Texture::TextureType::NORMAL:
				material.MaterialData.NormalTextureIndex = textureIndex;
				break;
			default:
				break;
			};

		}
	}
}

static void ProcessMaterials(
	Assets::Model& model,
	const aiScene* scene) {

	ResourceManager* rm = ResourceManager::Get();

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

		if (rm->GetMaterialIndex(materialName.C_Str()) == -1) {
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

			LoadMaterialTextures(material, newMaterial, model.MaterialPath);
			
			rm->AddMaterial(newMaterial);
		}
	}
}

void CompileMesh(Assets::Model& model) {

	std::vector<Assets::Vertex> vertices;
	std::vector<uint32_t> indices;

	float min_x = std::numeric_limits<float>::max();
	float max_x = std::numeric_limits<float>::min();

	float min_y = std::numeric_limits<float>::max();
	float max_y = std::numeric_limits<float>::min();
	
	float min_z = std::numeric_limits<float>::max();
	float max_z = std::numeric_limits<float>::min();

	ResourceManager* rm = ResourceManager::Get();

	for (auto& mesh : model.Meshes) {

		int materialIndex = rm->GetMaterialIndex(mesh.MaterialName);

		if (materialIndex == -1) {
			std::string customMaterialName = "Custom_Material_" + model.Name;
			rm->AddMaterial(Material(customMaterialName));
			mesh.MaterialName = customMaterialName;
			mesh.MaterialIndex = rm->GetLastMaterialIndex();
		}
		else {
			mesh.MaterialIndex = materialIndex;
		}

		mesh.IndexOffset = indices.size();
		mesh.VertexOffset = vertices.size();

		indices.insert(indices.end(), mesh.Indices.begin(), mesh.Indices.end());
		vertices.insert(vertices.end(), mesh.Vertices.begin(), mesh.Vertices.end());

		if (rm->GetMaterial(materialIndex).MaterialData.Diffuse.a < 1.0f) {
			mesh.PSOFlags |= PSOFlags::tTransparent;
			mesh.PSOFlags |= PSOFlags::tTwoSided;
		} else {
			mesh.PSOFlags |= PSOFlags::tOpaque;
		}

		float mesh_max_x = std::numeric_limits<float>::min();
		float mesh_min_x = std::numeric_limits<float>::max();
		
		float mesh_max_y = std::numeric_limits<float>::min();
		float mesh_min_y = std::numeric_limits<float>::max();
	
		float mesh_max_z = std::numeric_limits<float>::min();
		float mesh_min_z = std::numeric_limits<float>::max();

		for (const auto& vertex: mesh.Vertices) {
			max_x = std::max(vertex.pos.x, max_x);
			max_y = std::max(vertex.pos.y, max_y);
			max_z = std::max(vertex.pos.z, max_z);

			min_x = std::min(vertex.pos.x, min_x);
			min_y = std::min(vertex.pos.y, min_y);
			min_z = std::min(vertex.pos.z, min_z);

			mesh_max_x = std::max(vertex.pos.x, mesh_max_x);
			mesh_max_y = std::max(vertex.pos.y, mesh_max_y);
			mesh_max_z = std::max(vertex.pos.z, mesh_max_z);

			mesh_min_x = std::min(vertex.pos.x, mesh_min_x);
			mesh_min_y = std::min(vertex.pos.y, mesh_min_y);
			mesh_min_z = std::min(vertex.pos.z, mesh_min_z);
		}

		mesh.PivotVector = glm::vec3((mesh_max_x + mesh_min_x) / 2, (mesh_max_y + mesh_min_y) / 2, (mesh_max_z + mesh_min_z) / 2);
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
}

std::shared_ptr<Assets::Model> ModelLoader::LoadModel(const std::string& path) {

	Timestep begin = glfwGetTime();

	std::shared_ptr<Assets::Model> model = std::make_shared<Assets::Model>();
	model->ModelPath = path.c_str();
	model->MaterialPath = Helper::get_directory(path);
	model->Name = Helper::get_directory_name(path);
	
	static std::unordered_map<std::string, int> loadedFileNames;

	if (loadedFileNames[model->Name] > 0) {
		model->Name = model->Name + "_" + std::to_string(loadedFileNames[model->Name]);
	}
		
	loadedFileNames[model->Name]++;

	const aiScene* scene = aiImportFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);

	assert(scene && scene->HasMeshes());

	ProcessNode(*model.get(), scene->mRootNode, scene);
	ProcessMaterials(*model.get(), scene);
	CompileMesh(*model.get());

	Timestep end = glfwGetTime();

	std::cout << "Loading time: " << end.GetSeconds() - begin.GetSeconds() << " | Model: " << model->Name << '\n';

	return model;
}

std::shared_ptr<Assets::Model> ModelLoader::LoadModel(ModelType modelType, glm::vec3 position, float size) {
	std::shared_ptr<Assets::Model> model = std::make_shared<Assets::Model>();

	if (modelType == ModelType::CUBE) {
		static int cubeIdx = 0;
	
		model->Meshes = Assets::MeshGenerator::GenerateCubeMesh(position, size);
		model->Name = "Cube_" + std::to_string(cubeIdx++);
		model->Transformations.translation = position;

		CompileMesh(*model.get());

		return model;
	}
}
