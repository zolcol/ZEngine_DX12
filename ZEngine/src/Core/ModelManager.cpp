#include "pch.h"
#include "ModelManager.h"
#include "Renderer/Buffer.h"
#include "ModelLoader.h"
#include "Renderer/Texture2D.h"
#include "Renderer/CommandContext.h"
#include "Renderer/DescriptorManager.h"
#include "Model.h"



void ModelManager::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager)
{
	m_Device = device;
	m_CommandContext = commandContext;
	m_DescriptorManager = descriptorManager;

	m_VertexBuffer = std::make_unique<Buffer>();
	m_IndexBuffer = std::make_unique<Buffer>();
	m_MaterialBuffer = std::make_unique<Buffer>();

	m_VertexBuffer->Init(device, sizeof(VertexData) * MAX_VERTICES, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
	m_IndexBuffer->Init(device, sizeof(uint32_t) * MAX_INDICES, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
	m_MaterialBuffer->Init(m_Device, MAX_MATERIALS * sizeof(MaterialData), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);

	m_DescriptorManager->CreateRootSRV(m_MaterialBuffer.get(), 0, 2, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
}

void ModelManager::UploadMaterialBuffer()
{
	if (m_Materials.empty())
	{
		return;
	}

	m_MaterialBuffer->UploadData(m_Device, m_CommandContext, m_Materials.data(), m_Materials.size() * sizeof(MaterialData), 0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	m_IsMaterialUpdated = true;
}

Model* ModelManager::InitModel(const std::string& filePath, DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 rotation)
{
	std::vector<MeshLoaderData> meshLoaderDatas = LoadModel(filePath, scale, rotation);

	if (meshLoaderDatas.empty())
	{
		ENGINE_ERROR("ModelManager: Failed to load model or model is empty: {}", filePath);
		return nullptr;
	}

	std::unique_ptr<Model> model = std::make_unique<Model>();

	for (const auto& meshLoaderData : meshLoaderDatas)
	{
		if (meshLoaderData.indices.empty() || meshLoaderData.vertices.empty())
		{
			continue;
		}

		if (m_CurrentVertexLocation + meshLoaderData.vertices.size() > MAX_VERTICES ||
			m_CurrentIndexLocation + meshLoaderData.indices.size() > MAX_INDICES)
		{
			ENGINE_ERROR("ModelManager Buffer Overflow! Cannot load mesh from: {}. Try increasing MAX_VERTICES or MAX_INDICES.", filePath);
			break;
		}

		Mesh mesh;
		mesh.indexCount = meshLoaderData.indices.size();
		mesh.startIndexLocation = m_CurrentIndexLocation;
		mesh.startVertexLocation = m_CurrentVertexLocation;
		mesh.materialIndex = InitMaterial(meshLoaderData.material);
		model->AddMesh(mesh);

		// Update Vertices, Indices
		m_VertexBuffer->UploadData(m_Device, m_CommandContext,
			meshLoaderData.vertices.data(),
			meshLoaderData.vertices.size() * sizeof(VertexData),
			m_CurrentVertexLocation * sizeof(VertexData),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		);

		m_IndexBuffer->UploadData(m_Device, m_CommandContext,
			meshLoaderData.indices.data(),
			meshLoaderData.indices.size() * sizeof(uint32_t),
			m_CurrentIndexLocation * sizeof(uint32_t),
			D3D12_RESOURCE_STATE_INDEX_BUFFER
		);

		m_CurrentIndexLocation += meshLoaderData.indices.size();
		m_CurrentVertexLocation += meshLoaderData.vertices.size();
	}

	if (model->GetMeshes().empty())
	{
		ENGINE_ERROR("ModelManager: Model has no valid meshes after processing: {}", filePath);
		return nullptr;
	}

	m_Models.push_back(std::move(model));

	return m_Models.back().get();
}


uint32_t ModelManager::InitMaterial(const MaterialLoaderData& materialData)
{
	MaterialData material;

	auto LoadTexture = [&](const std::string& filePath, DXGI_FORMAT format, TextureType type) -> uint32_t
	{
		if (!filePath.empty() && m_LoadedFilePaths.contains(filePath))
		{
			return m_LoadedFilePaths[filePath];
		}

		std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>();
		texture->Init(m_Device, m_CommandContext, m_DescriptorManager, filePath, format, type);

		uint32_t srvIndex = texture->GetSRVIndex();
		
		if (!filePath.empty())
		{
			m_LoadedFilePaths[filePath] = srvIndex;
		}
		
		m_Textures.push_back(std::move(texture));
		return srvIndex;
	};

	material.albedoSRVIndex   = LoadTexture(materialData.AlbedoFilePath, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, ALBEDO);
	material.normalSRVIndex   = LoadTexture(materialData.NormalFilePath, DXGI_FORMAT_R8G8B8A8_UNORM, NORMAL);
	material.ormSRVIndex      = LoadTexture(materialData.ORMFilePath, DXGI_FORMAT_R8G8B8A8_UNORM, ORM);
	material.emissiveSRVIndex = LoadTexture(materialData.emissiveFilePath, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, EMISSIVE);
	
	m_Materials.push_back(material);

	m_IsMaterialUpdated = false;
	return m_Materials.size() - 1;
}
