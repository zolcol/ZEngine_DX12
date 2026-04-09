#include "pch.h"
#include "DescriptorAllocator.h"

bool DescriptorAllocator::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = numDescriptors;
	desc.Type = heapType;

	if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
	{
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}
	else
	{
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	}

	CHECK(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_DescriptorHeap)));

	m_HeapStartCpuHandle = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_DescriptorHandleSize = device->GetDescriptorHandleIncrementSize(heapType);
	MAX_DESCRIPTORS = numDescriptors;

	return true;
}

bool DescriptorAllocator::Allocate(uint32_t& index, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
{
	if (m_CurrentIndex >= MAX_DESCRIPTORS)
	{
		return false;
	}
	index = m_CurrentIndex;
	CD3DX12_CPU_DESCRIPTOR_HANDLE currentCpuHandle(m_HeapStartCpuHandle, m_CurrentIndex, m_DescriptorHandleSize);
	cpuHandle = currentCpuHandle;

	m_CurrentIndex += 1;

	return true;
}
