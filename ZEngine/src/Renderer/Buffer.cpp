#include "pch.h"
#include "Buffer.h"
#include "CommandContext.h"

Buffer::Buffer() = default;
Buffer::~Buffer() = default;

bool Buffer::Init(ID3D12Device* device, uint32_t bufferSize, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState)
{
	m_BufferSize = bufferSize;
	m_HeapType = heapType;
	m_CurrentState = initialState;

	if (bufferSize <= 0)
	{
		ENGINE_ERROR("Buffer Size is Invalid!!!");
		return false;
	}

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
		ENGINE_ERROR("Failed To Upload Buffer Data <= Reason: Offset: {}, UploadSize: {} > m_BufferSize: {}", offset, uploadSize, m_BufferSize);
		return;
	}

	if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		UploadDataToBuffer(device, m_Buffer.Get(), pSrcData, uploadSize, offset);
		return;
	}
	if (m_HeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		CreateStagingBuffer(device, uploadSize);
		UploadDataToBuffer(device, m_StagingBuffer.Get(), pSrcData, uploadSize, 0);
		CopyStagingToBuffer(device, commandContext, uploadSize, offset, afterState);
		m_StagingBuffer.Reset();
		return;
	}

	ENGINE_ERROR("Dont Have Code For This HeapType - HeaptypeID: {}", (int)m_HeapType);
}

void Buffer::UploadDataToBuffer(ID3D12Device* device, ID3D12Resource* dstBuffer, const void* srcData, uint32_t uploadSize, uint32_t offset)
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc = dstBuffer->GetDesc();
	
	uint32_t dstBufferSize = resourceDesc.Width;

	if (offset + uploadSize > dstBufferSize)
	{
		ENGINE_ERROR("Failed To Upload Buffer Data <= Reason: Offset: {}, UploadSize: {} > dstBufferSize: {}", offset, uploadSize, dstBufferSize);
		return;
	}

	void* bufferDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	CHECK(dstBuffer->Map(0, &readRange, &bufferDataBegin));

	memcpy((uint8_t*)bufferDataBegin + offset, srcData, uploadSize);

	CD3DX12_RANGE writeRange(offset, offset + uploadSize);
	dstBuffer->Unmap(0, &writeRange);
}

void Buffer::CreateStagingBuffer(ID3D12Device* device, uint32_t uploadSize)
{
	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

	CHECK(device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_StagingBuffer)
	));
}

void Buffer::CopyStagingToBuffer(ID3D12Device* device, CommandContext* commandContext, uint32_t uploadSize, uint32_t offset, D3D12_RESOURCE_STATES afterState)
{
	ID3D12GraphicsCommandList* cmdList = commandContext->BeginImmediateCommand();

	CD3DX12_RESOURCE_BARRIER barrier;
	if (m_CurrentState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_Buffer.Get(),
			m_CurrentState, D3D12_RESOURCE_STATE_COPY_DEST,
			0, D3D12_RESOURCE_BARRIER_FLAG_NONE
		);
		m_CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;
		cmdList->ResourceBarrier(1, &barrier);
	}

	cmdList->CopyBufferRegion(m_Buffer.Get(), offset, m_StagingBuffer.Get(), 0, uploadSize);

	if (m_CurrentState != afterState)
	{
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_Buffer.Get(),
			m_CurrentState, afterState,
			0, D3D12_RESOURCE_BARRIER_FLAG_NONE
		);

		m_CurrentState = afterState;
		cmdList->ResourceBarrier(1, &barrier);
	}

	commandContext->EndImmediateCommand();
}

