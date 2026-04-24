#include "pch.h"
#include "DescriptorManager.h"
#include "Buffer.h"

DescriptorManager::DescriptorManager() = default;
DescriptorManager::~DescriptorManager() = default;

bool DescriptorManager::Init(ID3D12Device* device, uint32_t frameCount)
{
	m_Device = device;
	m_FrameCount = frameCount;

	// Chuẩn bị mảng 2 chiều chứa địa chỉ GPU cho từng frame cho MỌI slot
	m_SlotAddresses.resize(frameCount);
	for (auto& frameSlots : m_SlotAddresses)
	{
		frameSlots.fill(0); // Mặc định là NULL address
	}

	// Khởi tạo các Heap khổng lồ
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100000);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);

	// Khởi tạo Static Sampler
	InitStaticSamplers();

	return true;
}

// =====================================================================
// Slot Assignment
// =====================================================================

void DescriptorManager::SetRootCBV(RootSlot slot, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		m_SlotAddresses[i][slot] = address;
	}
}

void DescriptorManager::SetRootCBVPerFrame(RootSlot slot, uint32_t frameIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	m_SlotAddresses[frameIndex][slot] = address;
}

void DescriptorManager::SetRootSRV(RootSlot slot, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	for (uint32_t i = 0; i < m_FrameCount; ++i)
	{
		m_SlotAddresses[i][slot] = address;
	}
}

void DescriptorManager::SetRootSRVPerFrame(RootSlot slot, uint32_t frameIndex, D3D12_GPU_VIRTUAL_ADDRESS address)
{
	m_SlotAddresses[frameIndex][slot] = address;
}

// =====================================================================
// Resource View Creation
// =====================================================================

uint32_t DescriptorManager::CreateCBV(Buffer* buffer)
{
	uint32_t index = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};

	if (!m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(index, cpuHandle))
		ENGINE_ERROR("Failed To Allocate CBV Descriptor!!!");

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvViewDesc{};
	cbvViewDesc.BufferLocation = buffer->GetGpuAddress();
	cbvViewDesc.SizeInBytes = buffer->GetBufferSize();

	m_Device->CreateConstantBufferView(&cbvViewDesc, cpuHandle);
	return index;
}

uint32_t DescriptorManager::CreateSRV(ID3D12Resource* texture, const CD3DX12_SHADER_RESOURCE_VIEW_DESC* srvDesc)
{
	uint32_t index = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};

	if (!m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(index, cpuHandle))
		ENGINE_ERROR("Failed To Allocate SRV Descriptor!!!");

	m_Device->CreateShaderResourceView(texture, srvDesc, cpuHandle);
	return index;
}

uint32_t DescriptorManager::CreateUAV(ID3D12Resource* texture, const CD3DX12_UNORDERED_ACCESS_VIEW_DESC* uavDesc)
{
	uint32_t index = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};

	if (!m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(index, cpuHandle))
		ENGINE_ERROR("Failed To Allocate UAV Descriptor!!!");

	m_Device->CreateUnorderedAccessView(texture, nullptr, uavDesc, cpuHandle);
	return index;
}

uint32_t DescriptorManager::CreateRTV(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* rtvDesc)
{
	uint32_t index = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};

	if (!m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].Allocate(index, cpuHandle))
		ENGINE_ERROR("Failed To Allocate RTV Descriptor!!!");

	m_Device->CreateRenderTargetView(resource, rtvDesc, cpuHandle);
	return index;
}

uint32_t DescriptorManager::CreateDSV(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc)
{
	uint32_t index = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};

	if (!m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].Allocate(index, cpuHandle))
		ENGINE_ERROR("Failed To Allocate DSV Descriptor!!!");

	m_Device->CreateDepthStencilView(resource, dsvDesc, cpuHandle);
	return index;
}

// =====================================================================
// Getters for CPU Handles (RTV/DSV)
// =====================================================================

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetRTVCPUHandle(uint32_t index)
{
	return m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].GetCpuHandle(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetDSVCPUHandle(uint32_t index)
{
	return m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].GetCpuHandle(index);
}

// ==========================================
// Binding & Rendering
// ==========================================

void DescriptorManager::BindDescriptors(ID3D12GraphicsCommandList* cmdList, int currentFrame)
{
	const auto& frameAddresses = m_SlotAddresses[currentFrame];

	// 1. Bind các Root CBVs (Slot 1, 2)
	if (frameAddresses[Slot_GlobalCBV] != 0)
		cmdList->SetGraphicsRootConstantBufferView(Slot_GlobalCBV, frameAddresses[Slot_GlobalCBV]);
	
	if (frameAddresses[Slot_ShadowCBV] != 0)
		cmdList->SetGraphicsRootConstantBufferView(Slot_ShadowCBV, frameAddresses[Slot_ShadowCBV]);

	// 2. Bind các Root SRVs (Slot 3, 4, 5)
	if (frameAddresses[Slot_MaterialSRV] != 0)
		cmdList->SetGraphicsRootShaderResourceView(Slot_MaterialSRV, frameAddresses[Slot_MaterialSRV]);

	if (frameAddresses[Slot_ObjectSRV] != 0)
		cmdList->SetGraphicsRootShaderResourceView(Slot_ObjectSRV, frameAddresses[Slot_ObjectSRV]);

	if (frameAddresses[Slot_LightSRV] != 0)
		cmdList->SetGraphicsRootShaderResourceView(Slot_LightSRV, frameAddresses[Slot_LightSRV]);

	// 3. Bind Heaps và Bindless Tables (Slot 6, 7, 8)
	BindDescriptorHeaps(cmdList);
}

void DescriptorManager::InitStaticSamplers()
{
	// Register s0: Linear Wrap (Dùng cho 3D Models, Môi trường)
	CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	// Register s1: Point Clamp (Dùng cho UI, Post-Processing, Pixel Art)
	CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	// Register s2: Shadow Sampler (Comparison)
	CD3DX12_STATIC_SAMPLER_DESC shadowSampler(
		2, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER
	);
	shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	shadowSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;

	m_StaticSamplers.push_back(linearWrap);
	m_StaticSamplers.push_back(pointClamp);
	m_StaticSamplers.push_back(shadowSampler);
}

void DescriptorManager::BindDescriptorHeaps(ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* heaps[] = { m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetDescriptorHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetBaseGpuHandle();

	// Bind CBV, SRV, UAV Tables vào đúng các slot cuối trong Root Signature (Slot 6, 7, 8)
	cmdList->SetGraphicsRootDescriptorTable(Slot_CBVTable, gpuStart);
	cmdList->SetGraphicsRootDescriptorTable(Slot_SRVTable, gpuStart);
	cmdList->SetGraphicsRootDescriptorTable(Slot_UAVTable, gpuStart);
}
