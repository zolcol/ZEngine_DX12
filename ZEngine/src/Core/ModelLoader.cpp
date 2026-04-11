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

		// Tangent (Cần thiết cho Normal Mapping sau này)
		if (mesh->HasTangentsAndBitangents())
		{
			vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
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

	// 3. Process Material (Albedo)
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiString texturePath;

		// --- DEBUG LOGGING: In ra tất cả các loại texture mà Assimp tìm thấy ---
		for (int i = aiTextureType_NONE; i <= aiTextureType_UNKNOWN; ++i)
		{
			if (material->GetTextureCount(static_cast<aiTextureType>(i)) > 0)
			{
				aiString path;
				material->GetTexture(static_cast<aiTextureType>(i), 0, &path);
			}
		}
		// -------------------------------------------------------------------------

		// Kiểm tra PBR Base Color (Thường gặp ở GLTF/GLB) trước, sau đó mới thử Diffuse truyền thống (OBJ/FBX)
		if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == aiReturn_SUCCESS ||
			material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
		{
			std::string relativePath = texturePath.C_Str();
			
			// Thay thế toàn bộ dấu gạch chéo ngược '\' thành dấu gạch chéo tới '/' để chuẩn hóa đường dẫn
			std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

			// Combine directory and relative path
			meshData.material.AlbedoFilePath = directory + "/" + relativePath;
		}

		if (meshData.material.AlbedoFilePath == "")
		{
			ENGINE_ERROR("Mesh Dont Have Albedo FilePath!!!");
		}
	}

	meshes.push_back(meshData);
}