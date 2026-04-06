#pragma once

class Fence;

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

	bool Init(ID3D12Device* device, uint32_t framesInFlight);

	ID3D12GraphicsCommandList* BeginImmediateCommand();
	void EndImmediateCommand();
private:
	ComPtr<ID3D12CommandQueue> m_CommandQueue;
	std::vector<FrameCommandResource> m_FrameCommandResources;

	// One Time Command
	std::unique_ptr<Fence> m_ImmediateFence;
	uint64_t m_ImmediateFenceValue = 0;
	ComPtr<ID3D12CommandAllocator> m_ImmediateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_ImmediateCommandList;
};
