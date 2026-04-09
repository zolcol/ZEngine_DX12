#pragma once

// =====================================================================
// Lớp quản lý cấp phát (Allocation) cho một Descriptor Heap cụ thể
// Hoạt động theo cơ chế Linear Allocator (chỉ tăng, chưa có Free List)
// =====================================================================
class DescriptorAllocator
{
public:
	DescriptorAllocator() = default;
	~DescriptorAllocator() = default;

	// ==========================================
	// Khởi tạo
	// ==========================================
	// heapType: Loại Heap (CBV_SRV_UAV, DSV, RTV, SAMPLER)
	// numDescriptors: Tổng số slot muốn cấp phát
	bool Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);

	// ==========================================
	// Cấp phát
	// ==========================================
	// Lấy ra 1 slot trống trong Heap. Trả về index và CPU Handle của slot đó.
	bool Allocate(uint32_t& outIndex, D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle);

	// ==========================================
	// Getters
	// ==========================================
	int GetCurrentIndex() const { return m_CurrentIndex; }
	ID3D12DescriptorHeap* GetDescriptorHeap() const { return m_DescriptorHeap.Get(); }
	
	// Dùng để Bind nguyên cả cái Heap vào CommandList
	D3D12_GPU_DESCRIPTOR_HANDLE GetBaseGpuHandle() const { return m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart(); }
	
	// Tính toán tự động địa chỉ CPU dựa vào index
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(int index) const 
	{ 
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_HeapStartCpuHandle, index, m_DescriptorHandleSize); 
	}

private:
	ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
	
	// Địa chỉ bắt đầu của Heap trên CPU (dùng làm mốc để tính offset)
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_HeapStartCpuHandle;
	
	// Kích thước của 1 slot (tùy thuộc vào phần cứng GPU và loại Heap)
	int m_DescriptorHandleSize = 0;
	
	uint32_t MAX_DESCRIPTORS = 0;
	int m_CurrentIndex = 0;
};
