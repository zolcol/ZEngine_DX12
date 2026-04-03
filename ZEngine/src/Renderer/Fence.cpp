#include "pch.h"
#include "Fence.h"

Fence::~Fence()
{
	CloseHandle(m_FenceEvent);
}

bool Fence::Init(ID3D12Device* device)
{
	CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
	
	m_FenceEvent = CreateEvent(nullptr, false, false, nullptr);

	if (m_FenceEvent == nullptr)
	{
		ENGINE_ERROR("FAILED TO CREATE FENCE EVENT");
		return false;
	}
	return true;
}

void Fence::Wait(uint64_t fenceValue)
{
	if (!IsCompleted(fenceValue))
	{
		CHECK(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
		WaitForSingleObject(m_FenceEvent, INFINITE);
	}
}

void Fence::Signal(ID3D12CommandQueue* commandQueue, uint64_t fenceValue)
{
	CHECK(commandQueue->Signal(m_Fence.Get(), fenceValue));
}

bool Fence::IsCompleted(uint64_t fenceValue)
{
	return m_Fence->GetCompletedValue() >= fenceValue;
}

void Fence::Flush(ID3D12CommandQueue* commandQueue, uint64_t fenceValue)
{
	fenceValue += 1;
	commandQueue->Signal(m_Fence.Get(), fenceValue);
	Wait(fenceValue);
	
}
