#include "pch.h"
#include "DescriptorAllocator.h"

bool DescriptorAllocator::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = numDescriptors;
	desc.Type = heapType;

	// Chỉ có CBV_SRV_UAV và SAMPLER mới cần (và được phép) Shader Visible
	// RTV và DSV tuyệt đối không được Shader Visible vì GPU chỉ dùng chúng ở giai đoạn OM (Output Merger)
	if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
	{
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}
	else
	{
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	}

	CHECK(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));

	// Lưu lại mốc bắt đầu và kích thước 1 bước nhảy (Increment Size)
	m_HeapStartCpuHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_DescriptorHandleSize = device->GetDescriptorHandleIncrementSize(heapType);
	MAX_DESCRIPTORS = numDescriptors;

	ENGINE_INFO("DescriptorAllocator Initialized: Type = {}, Capacity = {}", (int)heapType, numDescriptors);
	return true;
}

bool DescriptorAllocator::Allocate(uint32_t& outIndex, D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle)
{
	// Hết chỗ chứa
	if (m_CurrentIndex >= (int)MAX_DESCRIPTORS)
	{
		ENGINE_ERROR("DescriptorAllocator is OUT OF MEMORY! Max = {}", MAX_DESCRIPTORS);
		return false;
	}

	// Cấp phát
	outIndex = m_CurrentIndex;
	outCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_HeapStartCpuHandle, m_CurrentIndex, m_DescriptorHandleSize);

	// Tăng con trỏ lên cho lần cấp phát tiếp theo
	m_CurrentIndex += 1;

	return true;
}
