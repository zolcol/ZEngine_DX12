#include "pch.h"
#include "DescriptorManager.h"
#include "Buffer.h"

DescriptorManager::DescriptorManager() = default;
DescriptorManager::~DescriptorManager() = default;

bool DescriptorManager::Init(ID3D12Device* device, uint32_t frameCount)
{
	m_Device = device;
	m_FrameCount = frameCount;

	// Chuẩn bị mảng 2 chiều chứa địa chỉ GPU cho từng frame
	m_RootCBVsAddress.resize(frameCount);
	m_RootSRVsAddress.resize(frameCount);

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
// Root Parameter Setup
// =====================================================================

void DescriptorManager::CreateRootCBV(Buffer* buffer, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	// 1. Đăng ký Layout
	CD3DX12_ROOT_PARAMETER1 cbvParam;
	cbvParam.InitAsConstantBufferView(baseRegister, space, flags, visibility);
	m_RootCBVParams.push_back(cbvParam);
	
	// 2. Vì Buffer này là tĩnh (dùng chung cho mọi frame), copy địa chỉ của nó cho tất cả các slot frame
	for (auto& frameAddrs : m_RootCBVsAddress)
	{
		frameAddrs.push_back(buffer->GetGpuAddress());
	}
}

void DescriptorManager::CreateRootCBVPerFrame(const std::vector<std::unique_ptr<Buffer>>& buffers, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	if (buffers.size() != m_FrameCount)
	{
		ENGINE_FATAL("CreateRootCBVPerFrame: Buffers size ({}) must match frame count ({})!", buffers.size(), m_FrameCount);
		return;
	}

	// 1. Đăng ký Layout
	CD3DX12_ROOT_PARAMETER1 cbvParam;
	cbvParam.InitAsConstantBufferView(baseRegister, space, flags, visibility);
	m_RootCBVParams.push_back(cbvParam);

	// 2. Gán địa chỉ buffer thực tế của từng frame vào slot tương ứng
	for (size_t i = 0; i < m_FrameCount; ++i)
	{
		m_RootCBVsAddress[i].push_back(buffers[i]->GetGpuAddress());
	}
}

void DescriptorManager::CreateRootSRV(Buffer* buffer, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
	CD3DX12_ROOT_PARAMETER1 srvParam;
	srvParam.InitAsShaderResourceView(baseRegister, space, flags, visibility);

	m_RootSRVParams.push_back(srvParam);
	
	for (size_t i = 0; i < m_FrameCount; i++)
	{
		m_RootSRVsAddress[i].push_back(buffer->GetGpuAddress());
	}
}

void DescriptorManager::CreateRootSRVPerFrame(const std::vector<std::unique_ptr<Buffer>>& buffers, 
	UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility /*= D3D12_SHADER_VISIBILITY_ALL*/)
{
	if (buffers.size() != m_FrameCount)
	{
		ENGINE_FATAL("CreateRootSRVPerFrame: Buffers size ({}) must match frame count ({})!", buffers.size(), m_FrameCount);
		return;
	}

	CD3DX12_ROOT_PARAMETER1 srvParam;
	srvParam.InitAsShaderResourceView(baseRegister, space, flags, visibility);

	m_RootSRVParams.push_back(srvParam);

	for (size_t i = 0; i < m_FrameCount; i++)
	{
		m_RootSRVsAddress[i].push_back(buffers[i]->GetGpuAddress());
	}
}

void DescriptorManager::CreateRootConstants(UINT num32BitValues, UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
	CD3DX12_ROOT_PARAMETER1 rootConstantParams;
	rootConstantParams.InitAsConstants(num32BitValues, shaderRegister, registerSpace, visibility);
	m_RootConstantParams.push_back(rootConstantParams);
}

void DescriptorManager::SetupStandardDescriptorTables()
{
	// Vị trí bắt đầu của CBV Param là ngay sau Root Constant Param
	m_CBVParamStartIndex = m_RootConstantParams.size();

	// Vị trí bắt đầu của SRV Param ngay sau CBV Params
	m_SRVParamStartIndex = m_CBVParamStartIndex + m_RootCBVParams.size();

	// Gộp CBV Param -> SRV Param -> Constant Param vào RootParam chính
	m_RootParams = m_RootConstantParams;
	m_RootParams.insert(m_RootParams.end(), m_RootCBVParams.begin(), m_RootCBVParams.end());
	m_RootParams.insert(m_RootParams.end(), m_RootSRVParams.begin(), m_RootSRVParams.end());

	m_RootCBVParams.clear();
	m_RootSRVParams.clear();
	m_RootConstantParams.clear();

	// Ghi nhớ lại vị trí bắt đầu của 3 bảng Unbound trong mảng Params
	m_TableParamStartIndex = static_cast<uint32_t>(m_RootParams.size());

	// Thiết lập các cờ (Flags) tối ưu nhất cho Bindless
	// 1. CBV Table
	m_TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	// 2. SRV Table
	m_TableRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	// 3. UAV Table
	m_TableRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	// Đóng gói chúng vào RootParams
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_ROOT_PARAMETER1 tableParam;
		tableParam.InitAsDescriptorTable(1, &m_TableRanges[i], D3D12_SHADER_VISIBILITY_ALL);
		m_RootParams.push_back(tableParam);
	}
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
	if (index >= (uint32_t)m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].GetCurrentIndex())
	{
		ENGINE_ERROR("Invalid RTV Index: {}", index);
		return D3D12_CPU_DESCRIPTOR_HANDLE();
	}
	return m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].GetCpuHandle(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetDSVCPUHandle(uint32_t index)
{
	if (index >= (uint32_t)m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].GetCurrentIndex())
	{
		ENGINE_ERROR("Invalid DSV Index: {}", index);
		return D3D12_CPU_DESCRIPTOR_HANDLE();
	}
	return m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].GetCpuHandle(index);
}

// =====================================================================
// Frame Binding
// =====================================================================

void DescriptorManager::BindDescriptors(ID3D12GraphicsCommandList* cmdList, int currentFrame)
{
	// 1. Bind nhanh toàn bộ Root CBVs của frame hiện tại
	const auto& frameCBVAddresses = m_RootCBVsAddress[currentFrame];
	for (uint32_t i = 0; i < (uint32_t)frameCBVAddresses.size(); i++)
	{
		cmdList->SetGraphicsRootConstantBufferView(i + m_CBVParamStartIndex, frameCBVAddresses[i]);
	}

	// 2. Bind nhanh toàn bộ Root SRVs.
	const auto& frameSRVAddresses = m_RootSRVsAddress[currentFrame];
	for (uint32_t i = 0; i < (uint32_t)frameSRVAddresses.size(); i++)
	{
		cmdList->SetGraphicsRootShaderResourceView(i + m_SRVParamStartIndex, frameSRVAddresses[i]);
	}

	// 3. Bind Heaps và 3 bảng Unbound Tables
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

	m_StaticSamplers.push_back(linearWrap);
	m_StaticSamplers.push_back(pointClamp);
}

void DescriptorManager::BindDescriptorHeaps(ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* heaps[] = { m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetDescriptorHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetBaseGpuHandle();

	// Bind CBV, SRV, UAV Tables vào đúng các slot cuối trong Root Signature
	cmdList->SetGraphicsRootDescriptorTable(m_TableParamStartIndex, gpuStart);
	cmdList->SetGraphicsRootDescriptorTable(m_TableParamStartIndex + 1, gpuStart);
	cmdList->SetGraphicsRootDescriptorTable(m_TableParamStartIndex + 2, gpuStart);
}
