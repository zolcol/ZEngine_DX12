#include "pch.h"
#include "Buffer.h"
#include "CommandContext.h"
#include <Core/ImageLoader.h>

Buffer::Buffer() = default;
Buffer::~Buffer() = default;

bool Buffer::Init(ID3D12Device* device, uint32_t bufferSize, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState)
{
	if (bufferSize <= 0)
	{
		ENGINE_ERROR("Buffer Initialization Failed: Size must be greater than 0!");
		return false;
	}

	if (heapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	m_BufferSize = bufferSize;
	m_HeapType = heapType;
	m_CurrentState = initialState;

	CD3DX12_HEAP_PROPERTIES heapProp(heapType);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	CHECK(device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, initialState,
		nullptr,
		IID_PPV_ARGS(&m_Buffer)
	));
	
	return true;
}

void Buffer::UploadData(ID3D12Device* device, CommandContext* commandContext, const void* pSrcData, uint32_t uploadSize, uint32_t offset, D3D12_RESOURCE_STATES afterState)
{
	if (offset + uploadSize > m_BufferSize)
	{
		ENGINE_ERROR("Buffer Upload Failed: Out of bounds!");
		return;
	}

	if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		UploadDataToBuffer(device, m_Buffer.Get(), pSrcData, uploadSize, offset);
		return;
	}
	else if (m_HeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		CreateStagingBuffer(device, uploadSize);
		UploadDataToBuffer(device, m_StagingBuffer.Get(), pSrcData, uploadSize, 0);
		CopyStagingToBuffer(device, commandContext, uploadSize, offset, afterState);
		m_StagingBuffer.Reset();
		return;
	}
}

ID3D12GraphicsCommandList* Buffer::UpdateDataToTexture(
	ID3D12Device* device, 
	CommandContext* commandContext,
	const ImageData* imageData, 
	ID3D12Resource* dstTexture)
{
	if (m_HeapType != D3D12_HEAP_TYPE_UPLOAD)
	{
		ENGINE_ERROR("UpdateDataToTexture requires a Buffer with D3D12_HEAP_TYPE_UPLOAD!");
		return nullptr;
	}

	uint32_t subresourceCount = (uint32_t)(imageData->metaData.mipLevels * imageData->metaData.arraySize);
	
	// Chuẩn bị dữ liệu cho từng Subresource (mỗi tầng Mip/Array)
	std::vector<D3D12_SUBRESOURCE_DATA> subresources(subresourceCount);
	const DirectX::Image* images = imageData->scratchImage->GetImages();

	for (uint32_t i = 0; i < subresourceCount; i++)
	{
		subresources[i].pData = images[i].pixels;
		subresources[i].RowPitch = (LONG_PTR)images[i].rowPitch;
		subresources[i].SlicePitch = (LONG_PTR)images[i].slicePitch;
	}

	ID3D12GraphicsCommandList* cmdList = commandContext->BeginImmediateCommand();

	// Thực hiện lệnh chép dữ liệu từ Staging Buffer (m_Buffer) sang Texture Resource
	UpdateSubresources(
		cmdList,
		dstTexture, 
		m_Buffer.Get(), 
		0, 0, subresourceCount,
		subresources.data()
	);

	return cmdList;
}

void Buffer::UploadDataToBuffer(ID3D12Device* device, ID3D12Resource* dstBuffer, const void* srcData, uint32_t uploadSize, uint32_t offset)
{
	void* bufferDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	CHECK(dstBuffer->Map(0, &readRange, &bufferDataBegin));
	memcpy(static_cast<uint8_t*>(bufferDataBegin) + offset, srcData, uploadSize);
	CD3DX12_RANGE writeRange(offset, offset + uploadSize);
	dstBuffer->Unmap(0, &writeRange);
}

void Buffer::CreateStagingBuffer(ID3D12Device* device, uint32_t uploadSize)
{
	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
	CHECK(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_StagingBuffer)));
}

void Buffer::CopyStagingToBuffer(ID3D12Device* device, CommandContext* commandContext, uint32_t uploadSize, uint32_t offset, D3D12_RESOURCE_STATES afterState)
{
	ID3D12GraphicsCommandList* cmdList = commandContext->BeginImmediateCommand();
	Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->CopyBufferRegion(m_Buffer.Get(), offset, m_StagingBuffer.Get(), 0, uploadSize);
	Transition(cmdList, afterState);
	commandContext->EndImmediateCommand();
}

void Buffer::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
{
	if (m_CurrentState == newState) return;
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Buffer.Get(), m_CurrentState, newState);
	m_CurrentState = newState;
	cmdList->ResourceBarrier(1, &barrier);
}
