#include "pch.h"
#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

// Forward declarations for recursive functions
void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory);
void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory);

std::vector<MeshLoaderData> LoadModel(const std::string& filePath)
{
	std::vector<MeshLoaderData> meshes;
	Assimp::Importer importer;

	// Flags optimized for rendering pipelines (Triangulation, Left-Handed system, Tangents)
	const aiScene* scene = importer.ReadFile(filePath,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "ModelLoader Error: " << importer.GetErrorString() << std::endl;
		return meshes;
	}

	// Extract directory to resolve relative texture paths
	std::string directory = filePath.substr(0, filePath.find_last_of("/\\"));

	ProcessNode(scene->mRootNode, scene, meshes, directory);

	return meshes;
}

void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory)
{
	// Process all meshes in the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, meshes, directory);
	}

	// Recursively process child nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, meshes, directory);
	}
}

void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory)
{
	MeshLoaderData meshData;

	// Pre-allocate memory to avoid reallocation overhead
	meshData.vertices.reserve(mesh->mNumVertices);
	meshData.indices.reserve(mesh->mNumFaces * 3);

	// 1. Process Vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		VertexData vertex{}; // Assumes a default constructor is available

		// Position
		vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		// Normal
		if (mesh->HasNormals())
		{
			vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		}

		// Tangent & Bitangent Sign (Dùng cho Normal Mapping)
		if (mesh->HasTangentsAndBitangents())
		{
			aiVector3D n = mesh->mNormals[i];
			aiVector3D t = mesh->mTangents[i];
			aiVector3D b = mesh->mBitangents[i];

			// Tính vector Bitangent từ Normal và Tangent
			aiVector3D t_cross_n = t ^ n; // Assimp dùng operator ^ cho Cross Product

			// Dot product giữa Bitangent thực tế và vector tính toán để lấy dấu
			float dot = t_cross_n * b; // Assimp dùng operator * cho Dot Product

			float tangentSign = (dot < 0.0f) ? -1.0f : 1.0f;

			vertex.tangent = { t.x, t.y, t.z, tangentSign };
		}
		else
		{
			vertex.tangent = { 0.0f, 0.0f, 0.0f, 1.0f };
		}

		// Texture Coordinates (Using the first UV channel)
		if (mesh->mTextureCoords[0])
		{
			vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}

		meshData.vertices.push_back(vertex);
	}

	// 2. Process Indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			meshData.indices.push_back(face.mIndices[j]);
		}
	}

	// 3. Process Material
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		// Lambda helper để thử tìm texture theo danh sách các loại (aiTextureType) ưu tiên
		auto GetTexturePath = [&](std::initializer_list<aiTextureType> types) -> std::string
		{
			aiString texturePath;
			for (auto type : types)
			{
				if (material->GetTexture(type, 0, &texturePath) == aiReturn_SUCCESS)
				{
					std::string relativePath = texturePath.C_Str();
					std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
					return directory + "/" + relativePath;
				}
			}
			return "";
		};

		// 1. Albedo (Base Color)
		meshData.material.AlbedoFilePath = GetTexturePath({ aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE });

		// 2. Normal Map (Obj thường dùng HEIGHT hoặc NORMALS)
		meshData.material.NormalFilePath = GetTexturePath({ aiTextureType_NORMALS, aiTextureType_HEIGHT });

		// 3. ORM (Occlusion, Roughness, Metallic) - GLTF thường pack vào UNKNOWN, có khi map vào METALNESS
		meshData.material.ORMFilePath = GetTexturePath({ aiTextureType_UNKNOWN, aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS });

		// 4. Emissive Map
		meshData.material.emissiveFilePath = GetTexturePath({ aiTextureType_EMISSIVE });

		if (meshData.material.AlbedoFilePath.empty())
		{
			ENGINE_INFO("Mesh does not have an Albedo FilePath. Using default.");
		}
	}

	meshes.push_back(meshData);
}