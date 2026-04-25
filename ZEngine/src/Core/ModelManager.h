#pragma once

class Model;
class Buffer;
class Texture2D;
class CommandContext;
class DescriptorManager;
struct MaterialLoaderData;
class MipmapManager;

struct MaterialData
{
	uint32_t albedoSRVIndex;      // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
	uint32_t normalSRVIndex;      // DXGI_FORMAT_R8G8B8A8_UNORM
	uint32_t ormSRVIndex;         // DXGI_FORMAT_R8G8B8A8_UNORM (Occlusion, Roughness, Metallic)
	uint32_t emissiveSRVIndex;    // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB (Nếu có)
};

class ModelManager
{
public:
	ModelManager() = default;
	~ModelManager() = default;

	Buffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
	Buffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }

	bool IsMaterialUpdated() const { return m_IsMaterialUpdated; }

	MaterialData GetMaterial(uint32_t index) const { return m_Materials[index]; }

	void Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager);
	void UploadMaterialBuffer();
	
	Model* InitModel(const std::string& filePath, 
		DirectX::XMFLOAT3 scale = { 1, 1, 1 }, 
		DirectX::XMFLOAT3 rotation = { 0, 0, 0 });
private:
	const uint32_t MAX_VERTICES = 4000000;
	const uint32_t MAX_INDICES = 4000000;
	const uint32_t MAX_MATERIALS = 10000;

	bool m_IsMaterialUpdated = false;

	ID3D12Device* m_Device;
	CommandContext* m_CommandContext;
	DescriptorManager* m_DescriptorManager;

	std::vector<std::unique_ptr<Model>> m_Models;
	std::vector<std::unique_ptr<Texture2D>> m_Textures;
	std::vector<MaterialData> m_Materials;

	std::unordered_map<std::string, uint32_t> m_LoadedFilePaths;

	std::unique_ptr<Buffer> m_VertexBuffer;
	std::unique_ptr<Buffer> m_IndexBuffer;
	std::unique_ptr<Buffer> m_MaterialBuffer;

	uint32_t m_CurrentVertexLocation = 0;
	uint32_t m_CurrentIndexLocation = 0;

private:
	uint32_t InitMaterial(const MaterialLoaderData& materialData);
};
