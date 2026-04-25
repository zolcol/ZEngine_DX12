#include "pch.h"
#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

namespace fs = std::filesystem;

// Forward declarations for recursive functions
void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix);
void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix);

std::vector<MeshLoaderData> LoadModel(const std::string& filePath, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 rotation)
{
	std::vector<MeshLoaderData> meshes;

	if (!fs::exists(filePath))
	{
		ENGINE_ERROR("ModelLoader: File does not exist: {}", filePath);
		return meshes;
	}

	Assimp::Importer importer;
	using namespace DirectX;
	
	XMMATRIX bakeMatrix = XMMatrixScaling(scale.x, scale.y, scale.z) *
		XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(rotation.x), 
			XMConvertToRadians(rotation.y), 
			XMConvertToRadians(rotation.z)
		);

	const aiScene* scene = importer.ReadFile(filePath,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace |
		aiProcess_GenNormals |            // Đảm bảo luôn có Normal
		aiProcess_JoinIdenticalVertices); // Tối ưu vertex buffer

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		ENGINE_ERROR("Assimp Error: {} in file: {}", importer.GetErrorString(), filePath);
		return meshes;
	}

	std::string directory = fs::path(filePath).parent_path().string();

	ProcessNode(scene->mRootNode, scene, meshes, directory, bakeMatrix);

	return meshes;
}

void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene, meshes, directory, bakeMatrix);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], scene, meshes, directory, bakeMatrix);
	}
}

void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::vector<MeshLoaderData>& meshes, const std::string& directory, const DirectX::XMMATRIX& bakeMatrix)
{
	using namespace DirectX;
	MeshLoaderData meshData;

	meshData.vertices.reserve(mesh->mNumVertices);
	meshData.indices.reserve(mesh->mNumFaces * 3);

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		VertexData vertex{};

		XMVECTOR pos = XMVectorSet(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
		pos = XMVector3TransformCoord(pos, bakeMatrix);
		XMStoreFloat3(&vertex.position, pos);

		if (mesh->HasNormals())
		{
			XMVECTOR normal = XMVectorSet(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
			normal = XMVector3Normalize(XMVector3TransformNormal(normal, bakeMatrix));
			XMStoreFloat3(&vertex.normal, normal);
		}

		if (mesh->HasTangentsAndBitangents())
		{
			XMVECTOR tangent = XMVectorSet(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 0.0f);
			tangent = XMVector3Normalize(XMVector3TransformNormal(tangent, bakeMatrix));
			
			aiVector3D n = mesh->mNormals[i];
			aiVector3D t = mesh->mTangents[i];
			aiVector3D b = mesh->mBitangents[i];
			float dot = (t ^ n) * b;
			float tangentSign = (dot < 0.0f) ? -1.0f : 1.0f;

			XMStoreFloat3((XMFLOAT3*)&vertex.tangent.x, tangent); // Gán x,y,z an toàn
			vertex.tangent.w = tangentSign;
		}
		else
		{
			vertex.tangent = { 0.0f, 0.0f, 0.0f, 1.0f };
		}

		if (mesh->HasTextureCoords(0))
		{
			vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}

		meshData.vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			meshData.indices.push_back(face.mIndices[j]);
		}
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		auto GetTexturePath = [&](std::initializer_list<aiTextureType> types) -> std::string
		{
			aiString texturePath;
			for (auto type : types)
			{
				if (material->GetTexture(type, 0, &texturePath) == aiReturn_SUCCESS)
				{
					std::string relativePath = texturePath.C_Str();
					std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

					// BẮT BUỘC đổi đuôi sang .dds để khớp với quy trình của Engine
					size_t lastDot = relativePath.find_last_of('.');
					if (lastDot != std::string::npos)
					{
						relativePath = relativePath.substr(0, lastDot) + ".dds";
					}
					else
					{
						relativePath += ".dds";
					}

					return directory + "/" + relativePath;
				}
			}
			return "";
		};

		meshData.material.AlbedoFilePath = GetTexturePath({ aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE });
		meshData.material.NormalFilePath = GetTexturePath({ aiTextureType_NORMALS, aiTextureType_HEIGHT });
		meshData.material.ORMFilePath = GetTexturePath({ aiTextureType_UNKNOWN, aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS });
		meshData.material.emissiveFilePath = GetTexturePath({ aiTextureType_EMISSIVE });
	}

	meshes.push_back(meshData);
}