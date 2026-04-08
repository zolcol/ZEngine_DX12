#include "pch.h"
#include "DescriptorManager.h"

#include "Buffer.h"

DescriptorManager::DescriptorManager() = default;
DescriptorManager::~DescriptorManager() = default;

bool DescriptorManager::Init(ID3D12Device* device)
{
	m_Device = device;

	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100000);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);				// Tạo cho đủ index mảng KHÔNG DÙNG.
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
	
	return true;
}

uint32_t DescriptorManager::CreateCBV(Buffer* buffer)
{
	uint32_t index;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;

	if (!m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(index, cpuHandle))
	{
		ENGINE_ERROR("Failed To Allocate CBV Descriptor!!!");
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvViewDesc{};
	cbvViewDesc.BufferLocation = buffer->GetGpuAddress();
	cbvViewDesc.SizeInBytes = buffer->GetBufferSize();

	m_Device->CreateConstantBufferView(&cbvViewDesc, cpuHandle);

	return index;
}

void DescriptorManager::BindDescriptorHeap(ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* heaps[] = {
		m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetDescriptorHeap()
	};

	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->SetGraphicsRootDescriptorTable(1, m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetGpuHandle());	// CBV
	cmdList->SetGraphicsRootDescriptorTable(2, m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetGpuHandle());	// SRV
	cmdList->SetGraphicsRootDescriptorTable(3, m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetGpuHandle());	// UAV
}

