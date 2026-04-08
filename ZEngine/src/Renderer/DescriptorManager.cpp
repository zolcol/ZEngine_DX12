#include "pch.h"
#include "DescriptorManager.h"

#include "Buffer.h"

DescriptorManager::DescriptorManager() = default;
DescriptorManager::~DescriptorManager() = default;

bool DescriptorManager::Init(ID3D12Device* device, uint32_t frameCount)
{
	m_Device = device;
	m_FrameCount = frameCount;
	m_RootCBVsData.resize(frameCount);

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

void DescriptorManager::BindDescriptorHeap(int startParamIndex, ID3D12GraphicsCommandList* cmdList)
{
	ID3D12DescriptorHeap* heaps[] = {
		m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetDescriptorHeap()
	};

	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
	cmdList->SetGraphicsRootDescriptorTable(startParamIndex	   , m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetGpuHandle());	// CBV
	cmdList->SetGraphicsRootDescriptorTable(startParamIndex + 1, m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetGpuHandle());	// SRV
	cmdList->SetGraphicsRootDescriptorTable(startParamIndex + 2, m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetGpuHandle());	// UAV
}

void DescriptorManager::FrameDescriptorBind(ID3D12GraphicsCommandList* cmdList, int currentFrame)
{
	const std::vector<RootCBVData>& cbvFramedData = m_RootCBVsData[currentFrame];
	for (size_t i = 0; i < cbvFramedData.size(); i++)
	{
		cmdList->SetGraphicsRootConstantBufferView(i, cbvFramedData[i].GetGpuAddress());
	}

	BindDescriptorHeap(cbvFramedData.size(), cmdList);
}

void DescriptorManager::CreateRootCBV(Buffer* buffer, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	for (auto& rootCBVFrameData : m_RootCBVsData)
	{
		RootCBVData cbvData(buffer->GetGpuAddress(), baseRegister, space, flags, visibility);
		rootCBVFrameData.push_back(cbvData);
	}

	m_RootParams.push_back(m_RootCBVsData[0].back().GetParam());
}

void DescriptorManager::CreateRootCBVPerFrame(const std::vector<Buffer*>& buffers, UINT baseRegister, UINT space, D3D12_ROOT_DESCRIPTOR_FLAGS flags, D3D12_SHADER_VISIBILITY visibility)
{
	if (buffers.size() != m_FrameCount)
	{
		ENGINE_FATAL("Buffers Size Must Equal FramesInFlight!!!");
		return;
	}

	for (size_t i = 0; i < m_FrameCount; i++)
	{
		RootCBVData cbvData(buffers[i]->GetGpuAddress(), baseRegister, space, flags, visibility);
		m_RootCBVsData[i].push_back(cbvData);
	}

	m_RootParams.push_back(m_RootCBVsData[0].back().GetParam());
}

