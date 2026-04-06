#include "pch.h"
#include "VertexBuffer.h"
#include "CommandContext.h"

VertexBuffer::VertexBuffer() = default;
VertexBuffer::~VertexBuffer() = default;

bool VertexBuffer::Init(ID3D12Device* device, CommandContext * commandContext, const std::vector<VertexData>& vertices)
{
	if (vertices.empty())
	{
		ENGINE_ERROR("Vertex Data Is EMPTY!!!");
		return false;
	}

	CreateBuffer(device, vertices);
	CopyStagingToVertexBuffer(device, commandContext);

	m_StagingBuffer.Reset();

	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = m_BufferSize;
	m_VertexBufferView.StrideInBytes = sizeof(VertexData);
	 
	return true;
}

void VertexBuffer::CreateBuffer(ID3D12Device* device, const std::vector<VertexData>& vertices)
{
	m_VertexCount = static_cast<uint32_t>(vertices.size());
	m_BufferSize = m_VertexCount * sizeof(VertexData);

	CD3DX12_HEAP_PROPERTIES staginHeapProp(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC StagingBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_BufferSize);

	CHECK(device->CreateCommittedResource(
		&staginHeapProp, D3D12_HEAP_FLAG_NONE,
		&StagingBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_StagingBuffer)
	));

	// Copy Vertex Data CPU->GPU
	void* vertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	CHECK(m_StagingBuffer->Map(0, &readRange, &vertexDataBegin));

	memcpy(vertexDataBegin, vertices.data(), m_BufferSize);

	CD3DX12_RANGE writeRange(0, m_BufferSize);
	m_StagingBuffer->Unmap(0, &writeRange);

	// Create VertexBuffer
	CD3DX12_HEAP_PROPERTIES vertexBufferHeapProp(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_BufferSize);

	CHECK(device->CreateCommittedResource(
		&vertexBufferHeapProp, D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc ,D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr, 
		IID_PPV_ARGS(&m_VertexBuffer)
	));
}

void VertexBuffer::CopyStagingToVertexBuffer(ID3D12Device* device, CommandContext* commandContext)
{
	ID3D12GraphicsCommandList* cmdList = commandContext->BeginImmediateCommand();

	cmdList->CopyBufferRegion(m_VertexBuffer.Get(), 0, m_StagingBuffer.Get(), 0, m_BufferSize);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_VertexBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		0,
		D3D12_RESOURCE_BARRIER_FLAG_NONE
	);

	cmdList->ResourceBarrier(1, &barrier);

	commandContext->EndImmediateCommand();
}
