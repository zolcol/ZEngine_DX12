#include "pch.h"
#include "Texture2D.h"
#include <Core/ImageLoader.h>
#include "DescriptorManager.h"
#include "Buffer.h"
#include "CommandContext.h"

bool Texture2D::Init(ID3D12Device* device, CommandContext* commandContext, DescriptorManager* descriptorManager, const std::string& filePath, DXGI_FORMAT format)
{
	ImageData image(filePath);

	if (!image.pixels) return false;

	// Create Texture Resource
	CD3DX12_RESOURCE_DESC texDesc;
	texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format, image.width, image.height,
		1, 0,
		1, 0,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT_UNKNOWN
	);

	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);

	CHECK(device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_Resource)
	));

	UINT64 stagingBufferSize = GetRequiredIntermediateSize(m_Resource.Get(), 0, 1);

	Buffer stagingBuffer;
	stagingBuffer.Init(device, stagingBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	ID3D12GraphicsCommandList* cmdList = stagingBuffer.UpdateDataToTexture(commandContext, image.pixels, m_Resource.Get(), image.width, image.height);

	CD3DX12_RESOURCE_BARRIER barrier;
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Resource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		0,
		D3D12_RESOURCE_BARRIER_FLAG_NONE
	);

	cmdList->ResourceBarrier(1, &barrier);

	commandContext->EndImmediateCommand();

	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format);

	m_SRVIndex = descriptorManager->CreateSRV(m_Resource.Get(), &srvDesc);

	return true;
}
