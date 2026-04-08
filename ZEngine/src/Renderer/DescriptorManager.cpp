#include "pch.h"
#include "DescriptorManager.h"
#include "Buffer.h"

DescriptorManager::DescriptorManager() = default;
DescriptorManager::~DescriptorManager() = default;

bool DescriptorManager::Init(ID3D12Device* device, uint32_t frameCount)
{
	m_Device = device;
	m_FrameCount = frameCount;

	// Khởi tạo không gian lưu trữ địa chỉ CBV cho từng frame
	m_RootCBVsAddress.resize(frameCount);

	// Khởi tạo các Heap Allocators
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100000);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
	m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);

	return true;
}

void DescriptorManager::SetupStandardDescriptorTables()
{
	// Lưu lại index bắt đầu của các bảng Unbound
	m_TableParamStartIndex = (uint32_t)m_RootParams.size();

	// 1. CBV Table
	m_TableRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	// 2. SRV Table
	m_TableRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	// 3. UAV Table
	m_TableRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UINT_MAX, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

	// Đưa 3 bảng vào RootParams
	for (int i = 0; i < 3; ++i)
	{
		CD3DX12_ROOT_PARAMETER1 tableParam;
		tableParam.InitAsDescriptorTable(1, &m_TableRanges[i], D3D12_SHADER_VISIBILITY_ALL);
		m_RootParams.push_back(tableParam);
	}
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

void DescriptorManager::FrameDescriptorBind(ID3D12GraphicsCommandList* cmdList, int currentFrame)
{
	// 1. Bind các Root CBVs cho frame này
	const auto& frameAddresses = m_RootCBVsAddress[currentFrame];
	for (uint32_t i = 0; i < (uint32_t)frameAddresses.size(); ++i)
	{
		cmdList->SetGraphicsRootConstantBufferView(i, frameAddresses[i]);
	}

	// 2. Bind Descriptor Heaps và các Unbound Tables
	BindDescriptorHeaps(cmdList);
}

void DescriptorManager::BindDescriptorHeaps(ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* heaps[] = { m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetDescriptorHeap() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetGpuHandle();

	// Bind CBV, SRV, UAV Tables vào đúng các slot cuối trong Root Signature
	cmdList->SetGraphicsRootDescriptorTable(m_TableParamStartIndex, gpuStart);
	cmdList->SetGraphicsRootDescriptorTable(m_TableParamStartIndex + 1, gpuStart);
	cmdList->SetGraphicsRootDescriptorTable(m_TableParamStartIndex + 2, gpuStart);
}

void DescriptorManager::CreateRootCBV(Buffer* buffer, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	// Đăng ký Layout (Chỉ làm 1 lần)
	CD3DX12_ROOT_PARAMETER1 cbvParam;
	cbvParam.InitAsConstantBufferView(baseRegister, space, flags, visibility);
	m_RootParams.push_back(cbvParam);

	// Lưu trữ giá trị cho tất cả các frame (Vì là Buffer cố định)
	for (auto& frameAddrs : m_RootCBVsAddress)
	{
		frameAddrs.push_back(buffer->GetGpuAddress());
	}
}

void DescriptorManager::CreateRootCBVPerFrame(const std::vector<Buffer*>& buffers, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	if (buffers.size() != m_FrameCount)
	{
		ENGINE_FATAL("CreateRootCBVPerFrame: Buffers size must match frame count!");
		return;
	}

	// Đăng ký Layout (Chỉ làm 1 lần)
	CD3DX12_ROOT_PARAMETER1 cbvParam;
	cbvParam.InitAsConstantBufferView(baseRegister, space, flags, visibility);
	m_RootParams.push_back(cbvParam);

	// Lưu trữ giá trị địa chỉ thực tế cho từng frame tương ứng
	for (size_t i = 0; i < m_FrameCount; ++i)
	{
		m_RootCBVsAddress[i].push_back(buffers[i]->GetGpuAddress());
	}
}
