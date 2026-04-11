#pragma once

class Model;
class Buffer;
class Texture2D;
class CommandContext;
class DescriptorManager;
struct MaterialLoaderData;

struct MaterialData
{
	uint32_t albedoSRVIndex;
};

class ModelManager
{
public:
	ModelManager() = default;
	~ModelManager() = default;

	Buffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
	Buffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }

	MaterialData GetMaterial(uint32_t index) const { return m_Materials[index]; }

	void Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager);
	
	Model* InitModel(const std::string& filePath);
private:
	const uint32_t MAX_VERTICES = 500000;
	const uint32_t MAX_INDICES = 500000;

	ID3D12Device* m_Device;
	CommandContext* m_CommandContext;
	DescriptorManager* m_DescriptorManager;

	std::vector<std::unique_ptr<Model>> m_Models;
	std::vector<std::unique_ptr<Texture2D>> m_Textures;
	std::vector<MaterialData> m_Materials;

	std::unordered_map<std::string, uint32_t> m_LoadedFilePaths;

	std::unique_ptr<Buffer> m_VertexBuffer;
	std::unique_ptr<Buffer> m_IndexBuffer;



	uint32_t m_CurrentVertexLocation = 0;
	uint32_t m_CurrentIndexLocation = 0;

private:
	uint32_t InitMaterial(const MaterialLoaderData& materialData);
};
