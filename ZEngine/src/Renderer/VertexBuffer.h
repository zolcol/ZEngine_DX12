#pragma once

class CommandContext;

struct VertexData
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();
	
	const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_VertexBufferView; }
	uint32_t GetVertexCount() const { return m_VertexCount; }

	bool Init(ID3D12Device* device, CommandContext * commandContext, const std::vector<VertexData>& vertices);

private:
	ComPtr<ID3D12Resource> m_VertexBuffer;
	ComPtr<ID3D12Resource> m_StagingBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	uint32_t m_VertexCount;
	uint32_t m_BufferSize;

	void CreateBuffer(ID3D12Device* device, const std::vector<VertexData>& vertices);
	void CopyStagingToVertexBuffer(ID3D12Device* device, CommandContext * commandContext);
};
