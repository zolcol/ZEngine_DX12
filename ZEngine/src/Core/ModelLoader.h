#pragma once

struct MaterialLoaderData
{
	std::string AlbedoFilePath;
	std::string NormalFilePath;
	std::string ORMFilePath;
	std::string emissiveFilePath;
};

struct MeshLoaderData
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;

	MaterialLoaderData material;
};

std::vector<MeshLoaderData> LoadModel(const std::string& filePath, 
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f }, 
	DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f });

