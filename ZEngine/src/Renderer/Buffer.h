#pragma once

class CommandContext;

class Buffer
{
public:
	Buffer();
	~Buffer();

	// ==========================================
	// Initialization & Core Functionality
	// ==========================================
	bool Init(ID3D12Device* device, uint32_t bufferSize, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);
	
	void UploadData(ID3D12Device* device, CommandContext* commandContext, const void* pSrcData, uint32_t uploadSize, uint32_t offset, D3D12_RESOURCE_STATES afterState);

	ID3D12GraphicsCommandList* UpdateDataToTexture(CommandContext* commandContext, const void* pSrcPixels, ID3D12Resource* dstTexture, int textureWidth, int textureHeight, int pixelSizeInBytes = 4);

	// ==========================================
	// Getters
	// ==========================================
	uint32_t GetBufferSize() const { return m_BufferSize; }
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_Buffer->GetGPUVirtualAddress(); }
	ID3D12Resource* GetResource() const { return m_Buffer.Get(); }

private:
	// ==========================================
	// Internal Helpers
	// ==========================================
	void UploadDataToBuffer(ID3D12Device* device, ID3D12Resource* dstBuffer, const void* srcData, uint32_t uploadSize, uint32_t offset);
	void CreateStagingBuffer(ID3D12Device* device, uint32_t uploadSize);
	void CopyStagingToBuffer(ID3D12Device* device, CommandContext* commandContext, uint32_t uploadSize, uint32_t offset, D3D12_RESOURCE_STATES afterState);

private:
	// ==========================================
	// DirectX 12 Resources
	// ==========================================
	ComPtr<ID3D12Resource> m_Buffer;
	ComPtr<ID3D12Resource> m_StagingBuffer; // Chỉ dùng tạm thời khi upload lên DEFAULT heap
	
	// ==========================================
	// Buffer Properties
	// ==========================================
	uint32_t m_BufferSize = 0;
	D3D12_HEAP_TYPE m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
};
