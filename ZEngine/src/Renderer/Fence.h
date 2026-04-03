#pragma once

class Fence
{
public:
	Fence() = default;
	~Fence();

	bool Init(ID3D12Device* device);
	void Wait(uint64_t fenceValue);
	void Signal(ID3D12CommandQueue* commandQueue, uint64_t fenceValue);
	bool IsCompleted(uint64_t fenceValue);
	void Flush(ID3D12CommandQueue* commandQueue, uint64_t fenceValue);
private:
	ComPtr<ID3D12Fence> m_Fence;
	HANDLE m_FenceEvent;
};
