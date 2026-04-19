#include "pch.h"
#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

// Forward declarations for recursive functions
void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix);
void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix);

std::vector<MeshLoaderData> LoadModel(const std::string& filePath, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 rotation)
{
	std::vector<MeshLoaderData> meshes;
	Assimp::Importer importer;

	// Tính toán Ma trận Bake (Nướng) 
	using namespace DirectX;
	
	// Chuyển đổi từ ĐỘ sang RADIAN để dùng cho hàm XMMatrixRotationRollPitchYaw
	XMMATRIX bakeMatrix = XMMatrixScaling(scale.x, scale.y, scale.z) *
		XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(rotation.x), 
			XMConvertToRadians(rotation.y), 
			XMConvertToRadians(rotation.z)
		);

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

	ProcessNode(scene->mRootNode, scene, meshes, directory, bakeMatrix);

	return meshes;
}

void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix)
{
	// Process all meshes in the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, meshes, directory, bakeMatrix);
	}

	// Recursively process child nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, meshes, directory, bakeMatrix);
	}
}

void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix)
{
	using namespace DirectX;
	MeshLoaderData meshData;

	// Pre-allocate memory to avoid reallocation overhead
	meshData.vertices.reserve(mesh->mNumVertices);
	meshData.indices.reserve(mesh->mNumFaces * 3);

	// 1. Process Vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		VertexData vertex{};

		// --- BAKE POSITION ---
		XMVECTOR pos = XMVectorSet(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
		pos = XMVector3TransformCoord(pos, bakeMatrix); // TransformCoord áp dụng đầy đủ Translation/Rotation/Scale
		XMStoreFloat3(&vertex.position, pos);

		// --- BAKE NORMAL ---
		if (mesh->HasNormals())
		{
			XMVECTOR normal = XMVectorSet(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
			normal = XMVector3Normalize(XMVector3TransformNormal(normal, bakeMatrix)); // TransformNormal chỉ áp dụng Rotation/Scale
			XMStoreFloat3(&vertex.normal, normal);
		}

		// --- BAKE TANGENT ---
		if (mesh->HasTangentsAndBitangents())
		{
			XMVECTOR tangent = XMVectorSet(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 0.0f);
			tangent = XMVector3Normalize(XMVector3TransformNormal(tangent, bakeMatrix));
			
			// Xử lý bitangent sign
			aiVector3D n = mesh->mNormals[i];
			aiVector3D t = mesh->mTangents[i];
			aiVector3D b = mesh->mBitangents[i];
			aiVector3D t_cross_n = t ^ n;
			float dot = t_cross_n * b;
			float tangentSign = (dot < 0.0f) ? -1.0f : 1.0f;

			XMStoreFloat3((XMFLOAT3*)&vertex.tangent, tangent);
			vertex.tangent.w = tangentSign;
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