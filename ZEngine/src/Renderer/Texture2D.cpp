#include "pch.h"
#include "Texture2D.h"
#include <Core/ImageLoader.h>
#include "DescriptorManager.h"
#include "Buffer.h"
#include "CommandContext.h"

bool Texture2D::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, 
	const std::string& filePath, TextureType textureType)
{
	ImageData imageData(filePath);

	// Fallback sang texture mặc định nếu nạp ảnh thất bại
	if (!imageData.IsValid())
	{
		std::string typeStr;
		switch (textureType)
		{
		case ALBEDO:   typeStr = "ALBEDO"; break;
		case NORMAL:   typeStr = "NORMAL"; break;
		case ORM:      typeStr = "ORM"; break;
		case EMISSIVE: typeStr = "EMISSIVE"; break;
		default:       typeStr = "UNKNOWN"; break;
		}

		ENGINE_WARN("Failed to load {} texture: {}. Falling back to default.", typeStr, filePath);
		imageData = ImageData(DEFAULT_TEXTURE_PATH[textureType]);

		if (!imageData.IsValid())
		{
			ENGINE_FATAL("CRITICAL ERROR: Default texture missing: {}", DEFAULT_TEXTURE_PATH[textureType]);
			return false;
		}
	}

	m_MipLevels = (uint32_t)imageData.metaData.mipLevels;

	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		imageData.metaData.format,
		(UINT64)imageData.metaData.width,
		(UINT)imageData.metaData.height,
		(UINT16)imageData.metaData.arraySize,
		(UINT16)m_MipLevels,
		1, 0,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT_UNKNOWN
	);

	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&texDesc, D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)
	);

	if (FAILED(hr))
	{
		ENGINE_ERROR("Texture2D: Failed to create GPU Resource for: {} (HRESULT: 0x{:08X})", filePath, (unsigned int)hr);
		return false;
	}

	m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;

	// Copy dữ liệu từ RAM lên GPU Staging Buffer
	uint32_t subresourceCount = (uint32_t)(imageData.metaData.mipLevels * imageData.metaData.arraySize);
	UINT64 stagingBufferSize = GetRequiredIntermediateSize(m_Resource.Get(), 0, subresourceCount);

	Buffer stagingBuffer;
	if (!stagingBuffer.Init(device, (uint32_t)stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD))
	{
		ENGINE_ERROR("Texture2D: Failed to create staging buffer for: {}", filePath);
		return false;
	}

	ID3D12GraphicsCommandList* cmdList = stagingBuffer.UpdateDataToTexture(device, commandContext, &imageData, m_Resource.Get());
	if (cmdList)
	{
		this->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->EndImmediateCommand();
	}

	// Tạo Descriptor (SRV) cho Texture
	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format);
	m_SRVIndex = descriptorManager->CreateSRV(m_Resource.Get(), &srvDesc);

	return true;
}
