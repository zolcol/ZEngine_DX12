#include "pch.h"
#include "Buffer.h"
#include "CommandContext.h"

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
	
	ENGINE_INFO("Buffer Initialized: Size = {} bytes, HeapType = {}", bufferSize, (int)heapType);
	return true;
}

void Buffer::UploadData(ID3D12Device* device, CommandContext* commandContext, const void* pSrcData, uint32_t uploadSize, uint32_t offset, D3D12_RESOURCE_STATES afterState)
{
	// 1. Safety Check: Chống ghi tràn bộ nhớ
	if (offset + uploadSize > m_BufferSize)
	{
		ENGINE_ERROR("Buffer Upload Failed: Out of bounds! Offset: {} + UploadSize: {} > BufferSize: {}", offset, uploadSize, m_BufferSize);
		return;
	}

	// 2. Upload dựa trên loại Heap
	if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		// UPLOAD heap nằm trên RAM chia sẻ, CPU có thể map và copy thẳng vào
		UploadDataToBuffer(device, m_Buffer.Get(), pSrcData, uploadSize, offset);
		return;
	}
	else if (m_HeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		// DEFAULT heap nằm hoàn toàn trên VRAM của GPU, CPU không thể truy cập trực tiếp.
		// Bắt buộc phải thông qua một Staging Buffer (UPLOAD heap) làm trung gian.
		CreateStagingBuffer(device, uploadSize);
		UploadDataToBuffer(device, m_StagingBuffer.Get(), pSrcData, uploadSize, 0);
		CopyStagingToBuffer(device, commandContext, uploadSize, offset, afterState);
		
		// Giải phóng staging buffer ngay sau khi queue lệnh copy thành công
		m_StagingBuffer.Reset();
		return;
	}

	ENGINE_ERROR("Buffer Upload Failed: Unsupported HeapType ID: {}", (int)m_HeapType);
}

ID3D12GraphicsCommandList* Buffer::UpdateDataToTexture(
	CommandContext* commandContext, 
	const void* pSrcPixels, ID3D12Resource* dstTexture, 
	int textureWidth, int textureHeight, int pixelSizeInBytes)
{
	if (m_HeapType != D3D12_HEAP_TYPE_UPLOAD)
	{
		ENGINE_ERROR("Cant Upload Texture Data By Buffer Not Have Heap Type GPU_UPLOAD");
		return nullptr;
	}

	D3D12_SUBRESOURCE_DATA textureData{};
	textureData.pData = pSrcPixels;
	textureData.RowPitch = textureWidth * pixelSizeInBytes;
	textureData.SlicePitch = textureData.RowPitch * textureHeight;

	ID3D12GraphicsCommandList* cmdList = commandContext->BeginImmediateCommand();

	UpdateSubresources(
		cmdList,
		dstTexture,
		m_Buffer.Get(),
		0,
		0,
		1,
		&textureData
	);
	
	return cmdList;
}

void Buffer::UploadDataToBuffer(ID3D12Device* device, ID3D12Resource* dstBuffer, const void* srcData, uint32_t uploadSize, uint32_t offset)
{
	D3D12_RESOURCE_DESC resourceDesc = dstBuffer->GetDesc();
	uint32_t dstBufferSize = static_cast<uint32_t>(resourceDesc.Width);

	// Double check an toàn cho buffer đích (nhất là khi dùng staging buffer)
	if (offset + uploadSize > dstBufferSize)
	{
		ENGINE_ERROR("Buffer Map/Copy Failed: Out of bounds! Offset: {} + UploadSize: {} > DstBufferSize: {}", offset, uploadSize, dstBufferSize);
		return;
	}

	void* bufferDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0); // Đặt (0,0) vì CPU chỉ ghi, không đọc từ resource này
	
	CHECK(dstBuffer->Map(0, &readRange, &bufferDataBegin));

	memcpy(static_cast<uint8_t*>(bufferDataBegin) + offset, srcData, uploadSize);

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

	// Transition state của buffer đích sang COPY_DEST trước khi copy
	Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

	// Thực hiện lệnh copy trên GPU
	cmdList->CopyBufferRegion(m_Buffer.Get(), offset, m_StagingBuffer.Get(), 0, uploadSize);

	// Transition state của buffer đích về state theo yêu cầu của user
	Transition(cmdList, afterState);

	// Flush command và block CPU chờ đến khi copy xong (Sync immediate)
	commandContext->EndImmediateCommand();
}

void Buffer::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
{
	if (m_CurrentState == newState) return;

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_Buffer.Get(),
		m_CurrentState,
		newState
	);

	m_CurrentState = newState;
	cmdList->ResourceBarrier(1, &barrier);
}
