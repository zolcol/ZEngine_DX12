#include "pch.h"
#include "VertexBuffer.h"

bool VertexBuffer::Init(ID3D12Device* device, const std::vector<VertexData>& vertices)
{
	if (vertices.empty())
	{
		ENGINE_ERROR("Vertex Data Is EMPTY!!!");
		return false;
	}

	m_VertexCount = static_cast<uint32_t>(vertices.size());
	uint32_t bufferSize = m_VertexCount * sizeof(VertexData);

	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	CHECK(device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Buffer)
		));
	
	// Copy Vertex Data CPU->GPU
	void* vertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	CHECK(m_Buffer->Map(0, &readRange, &vertexDataBegin));

	memcpy(vertexDataBegin, vertices.data(), bufferSize);

	CD3DX12_RANGE writeRange(0, bufferSize);
	m_Buffer->Unmap(0, &writeRange);

	m_VertexBufferView.BufferLocation = m_Buffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = bufferSize;
	m_VertexBufferView.StrideInBytes = sizeof(VertexData);
	 
	return true;
}
