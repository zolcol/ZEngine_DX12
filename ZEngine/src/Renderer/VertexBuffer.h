#pragma once

struct VertexData
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

class VertexBuffer
{
public:
	VertexBuffer() = default;
	~VertexBuffer() = default;
	
	const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_VertexBufferView; }
	uint32_t GetVertexCount() const { return m_VertexCount; }

	bool Init(ID3D12Device* device, const std::vector<VertexData>& vertices);

private:
	ComPtr<ID3D12Resource> m_Buffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	uint32_t m_VertexCount;
};
