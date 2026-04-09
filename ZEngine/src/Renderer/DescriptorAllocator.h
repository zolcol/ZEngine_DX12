#pragma once

class DescriptorAllocator
{
public:
	DescriptorAllocator() = default;
	~DescriptorAllocator() = default;

	int GetCurrentIndex() const { return m_CurrentIndex; }
	ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_DescriptorHeap.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const { return m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(int index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_HeapStartCpuHandle, index, m_DescriptorHandleSize); }


	bool Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);
	bool Allocate(uint32_t & index, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle);
private:
	ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_HeapStartCpuHandle;
	uint32_t MAX_DESCRIPTORS = 0;
	int m_DescriptorHandleSize;
	int m_CurrentIndex = 0;
};
