#pragma once

struct FrameCommandResource
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	uint64_t fenceValue = 0;
};

class CommandContext
{
public:
	CommandContext() = default;
	~CommandContext() = default;

	ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue.Get(); }
	FrameCommandResource& GetFrameCommandResource(int currentFrame) { return m_FrameCommandResources[currentFrame]; }

	void Init(ID3D12Device* device, uint32_t framesInFlight);
private:
	ComPtr<ID3D12CommandQueue> m_CommandQueue;
	
	std::vector<FrameCommandResource> m_FrameCommandResources;
};
