#include "pch.h"
#include "CommandContext.h"
#include "Fence.h"

bool CommandContext::Init(ID3D12Device* device, uint32_t framesInFlight)
{
	m_FrameCommandResources.resize(framesInFlight);

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	CHECK(device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_CommandQueue)));

	for (size_t i = 0; i < framesInFlight; i++)
	{
		CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_FrameCommandResources[i].commandAllocator)));
		CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_FrameCommandResources[i].commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_FrameCommandResources[i].commandList)));

		// Đóng list lại vì chúng ta chưa record lệnh ngay lúc này
		m_FrameCommandResources[i].commandList->Close();
	}

	// KHởi tạo tài nguyên Immediate Command 
	m_ImmediateFence = std::make_unique<Fence>();
	if (!m_ImmediateFence->Init(device))
	{
		ENGINE_FATAL("Failed to create Immendiate Fence!!!");
		return false;
	}

	CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_ImmediateCommandAllocator)));
	CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_ImmediateCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_ImmediateCommandList)));
	
	m_ImmediateCommandList->Close();

	return true;
}

ID3D12GraphicsCommandList* CommandContext::BeginImmediateCommand()
{
	m_ImmediateCommandAllocator->Reset();
	m_ImmediateCommandList->Reset(m_ImmediateCommandAllocator.Get(), nullptr);
	
	return m_ImmediateCommandList.Get();
}

void CommandContext::EndImmediateCommand()
{
	m_ImmediateCommandList->Close();
	
	ID3D12CommandList* cmdList[] = {m_ImmediateCommandList.Get()};
	m_CommandQueue->ExecuteCommandLists(_countof(cmdList), cmdList);
	
	m_ImmediateFence->Signal(m_CommandQueue.Get(), ++m_ImmediateFenceValue);
	m_ImmediateFence->Wait(m_ImmediateFenceValue);
}
