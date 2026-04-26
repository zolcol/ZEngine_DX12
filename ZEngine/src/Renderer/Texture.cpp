#include "pch.h"
#include "Texture.h"
#include "Core/ImageLoader.h"
#include "Buffer.h"
#include "CommandContext.h"

uint32_t Texture::GetSRVIndex() const
{
	if (m_InitSRV)
	{
		return m_SRVIndex;
	}
	else
	{
		ENGINE_ERROR("Cannot Get SRV Index of texture have InitSRV = FALSE");
		return UINT32_MAX;
	}
}

void Texture::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
{
	if (m_CurrentState == newState) return;

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource.Get(),
		m_CurrentState,
		newState
	);

	m_CurrentState = newState;
	cmdList->ResourceBarrier(1, &barrier);
}

bool Texture::CreateResourceAndUpload(ID3D12Device* device, CommandContext* commandContext, const ImageData& imageData)
{
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
		ENGINE_ERROR("Texture: Failed to create GPU Resource (HRESULT: 0x{:08X})", (unsigned int)hr);
		return false;
	}

	m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;

	// Copy data from RAM to GPU via Staging Buffer
	uint32_t subresourceCount = (uint32_t)(imageData.metaData.mipLevels * imageData.metaData.arraySize);
	UINT64 stagingBufferSize = GetRequiredIntermediateSize(m_Resource.Get(), 0, subresourceCount);

	Buffer stagingBuffer;
	if (!stagingBuffer.Init(device, (uint32_t)stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD))
	{
		ENGINE_ERROR("Texture: Failed to create staging buffer");
		return false;
	}

	ID3D12GraphicsCommandList* cmdList = stagingBuffer.UpdateDataToTexture(device, commandContext, &imageData, m_Resource.Get());
	if (cmdList)
	{
		this->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->EndImmediateCommand();
		return true;
	}

	return false;
}
