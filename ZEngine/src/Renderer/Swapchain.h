#pragma once

struct SwapChainConfig
{
	HWND windowHandle = nullptr;
	IDXGIFactory4* factory = nullptr;
	ID3D12CommandQueue* commandQueue = nullptr;
	uint32_t width = 800;
	uint32_t height = 600;
	uint32_t frameCount = 3;
};

class Swapchain
{
public:
	Swapchain() = default;
	~Swapchain() = default;

	uint32_t GetCurrentBackBufferIndex() const { return m_Swapchain->GetCurrentBackBufferIndex(); }
	ID3D12Resource* GetBackBuffer(int currentFrame) const { return m_BackBuffers[currentFrame].Get(); }
	IDXGISwapChain4* GetSwapchain() const { return m_Swapchain.Get(); }

	void Init(const SwapChainConfig& config, ID3D12Device* device);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV();
private:
	SwapChainConfig m_SwapchainConfig;
	ComPtr<IDXGISwapChain4> m_Swapchain;

	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	uint32_t m_RtvDescriptorSize;
	std::vector<ComPtr<ID3D12Resource>> m_BackBuffers;
};
