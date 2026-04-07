#pragma once

class DescriptorAllocator
{
public:
	DescriptorAllocator() = default;
	~DescriptorAllocator() = default;

	ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_DescriptorHeap.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const { return m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart(); }

	bool Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);
	bool Allocate(uint32_t & index, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);
private:
	ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_HeapStartCpuHandle;
	uint32_t MAX_DESCRIPTORS = 0;
	int m_DescriptorHandleSize;
	int m_CurrentIndex = 0;
};
