#pragma once

struct MaterialLoaderData
{
	std::string AlbedoFilePath;
};

struct MeshLoaderData
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;

	MaterialLoaderData material;
};

std::vector<MeshLoaderData> LoadModel(const std::string& filePath);

