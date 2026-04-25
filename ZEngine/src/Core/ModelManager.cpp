#include "pch.h"
#include "ModelManager.h"
#include "Renderer/Buffer.h"
#include "ModelLoader.h"
#include "Renderer/Texture2D.h"
#include "Renderer/CommandContext.h"
#include "Renderer/DescriptorManager.h"
#include "Model.h"
#include "MipmapManager.h"

void ModelManager::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager)
{
	m_Device = device;
	m_CommandContext = commandContext;
	m_DescriptorManager = descriptorManager;

	m_VertexBuffer = std::make_unique<Buffer>();
	m_IndexBuffer = std::make_unique<Buffer>();
	m_MaterialBuffer = std::make_unique<Buffer>();

	bool success = true;
	success &= m_VertexBuffer->Init(device, sizeof(VertexData) * MAX_VERTICES, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
	success &= m_IndexBuffer->Init(device, sizeof(uint32_t) * MAX_INDICES, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
	success &= m_MaterialBuffer->Init(m_Device, MAX_MATERIALS * sizeof(MaterialData), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);

	if (!success)
	{
		ENGINE_FATAL("ModelManager: Failed to initialize GPU Buffers!");
		return;
	}

	m_DescriptorManager->SetRootSRV(Slot_MaterialSRV, m_MaterialBuffer->GetGpuAddress());
}

void ModelManager::UploadMaterialBuffer()
{
	if (m_Materials.empty()) return;

	m_MaterialBuffer->UploadData(m_Device, m_CommandContext, m_Materials.data(), (uint32_t)(m_Materials.size() * sizeof(MaterialData)), 0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	m_IsMaterialUpdated = true;
}

Model* ModelManager::InitModel(const std::string& filePath, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 rotation)
{
	std::vector<MeshLoaderData> meshLoaderDatas = LoadModel(filePath, scale, rotation);

	if (meshLoaderDatas.empty())
	{
		ENGINE_ERROR("ModelManager: Failed to load model: {}", filePath);
		return nullptr;
	}

	std::unique_ptr<Model> model = std::make_unique<Model>();

	for (const auto& meshLoaderData : meshLoaderDatas)
	{
		if (meshLoaderData.indices.empty() || meshLoaderData.vertices.empty()) continue;

		// Kiểm tra tràn buffer
		if (m_CurrentVertexLocation + meshLoaderData.vertices.size() > MAX_VERTICES ||
			m_CurrentIndexLocation + meshLoaderData.indices.size() > MAX_INDICES)
		{
			ENGINE_ERROR("ModelManager Buffer Overflow! Model: {}", filePath);
			return nullptr;
		}

		uint32_t matIndex = InitMaterial(meshLoaderData.material);
		if (matIndex == UINT32_MAX)
		{
			ENGINE_ERROR("ModelManager: Failed to initialize material for mesh in: {}", filePath);
			continue;
		}

		Mesh mesh;
		mesh.indexCount = (uint32_t)meshLoaderData.indices.size();
		mesh.startIndexLocation = m_CurrentIndexLocation;
		mesh.startVertexLocation = m_CurrentVertexLocation;
		mesh.materialIndex = matIndex;
		model->AddMesh(mesh);

		// Upload Vertices
		m_VertexBuffer->UploadData(m_Device, m_CommandContext,
			meshLoaderData.vertices.data(),
			(uint32_t)(meshLoaderData.vertices.size() * sizeof(VertexData)),
			(uint32_t)(m_CurrentVertexLocation * sizeof(VertexData)),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);

		// Upload Indices
		m_IndexBuffer->UploadData(m_Device, m_CommandContext,
			meshLoaderData.indices.data(),
			(uint32_t)(meshLoaderData.indices.size() * sizeof(uint32_t)),
			(uint32_t)(m_CurrentIndexLocation * sizeof(uint32_t)),
			D3D12_RESOURCE_STATE_INDEX_BUFFER
		);

		m_CurrentIndexLocation += (uint32_t)meshLoaderData.indices.size();
		m_CurrentVertexLocation += (uint32_t)meshLoaderData.vertices.size();
	}

	if (model->GetMeshes().empty()) return nullptr;

	m_Models.push_back(std::move(model));
	return m_Models.back().get();
}


uint32_t ModelManager::InitMaterial(const MaterialLoaderData& materialData)
{
	if (m_Materials.size() >= MAX_MATERIALS)
	{
		ENGINE_ERROR("ModelManager: Material limit reached ({})", MAX_MATERIALS);
		return UINT32_MAX;
	}

	MaterialData material;

	auto LoadTextureHelper = [&](const std::string& filePath, TextureType type) -> uint32_t
	{
		if (!filePath.empty() && m_LoadedFilePaths.contains(filePath))
		{
			return m_LoadedFilePaths[filePath];
		}

		std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
		if (!texture->Init(m_Device, m_CommandContext, m_DescriptorManager, filePath, type))
		{
			return 0; // SRV Index 0 thường là Default Texture
		}

		uint32_t srvIndex = texture->GetSRVIndex();
		if (!filePath.empty()) m_LoadedFilePaths[filePath] = srvIndex;
		
		m_Textures.push_back(std::move(texture));
		return srvIndex;
	};

	material.albedoSRVIndex   = LoadTextureHelper(materialData.AlbedoFilePath, ALBEDO);
	material.normalSRVIndex   = LoadTextureHelper(materialData.NormalFilePath, NORMAL);
	material.ormSRVIndex      = LoadTextureHelper(materialData.ORMFilePath, ORM);
	material.emissiveSRVIndex = LoadTextureHelper(materialData.emissiveFilePath, EMISSIVE);
	
	m_Materials.push_back(material);

	m_IsMaterialUpdated = false;
	return (uint32_t)(m_Materials.size() - 1);
}
