#pragma once

struct Mesh
{
	uint32_t indexCount;
	uint32_t startIndexLocation;
	uint32_t startVertexLocation;

	uint32_t materialIndex;
};

class Model
{
public:
	Model() = default;
	~Model() = default;

	const std::vector<Mesh>& GetMeshes() const { return m_Meshes; }

	void AddMesh(Mesh mesh) { m_Meshes.push_back(mesh); }
private:
	std::string m_ModelName;

	std::vector<Mesh> m_Meshes;
};