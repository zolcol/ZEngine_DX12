#include "pch.h"
#include "CommandContext.h"

void CommandContext::Init(ID3D12Device* device, uint32_t framesInFlight)
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
}
