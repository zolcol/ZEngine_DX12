#include "pch.h"
#include "Swapchain.h"

void Swapchain::Init(const SwapChainConfig& config, ID3D12Device* device)
{
	m_SwapchainConfig = config;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = config.frameCount;
	swapChainDesc.Width = config.width;
	swapChainDesc.Height = config.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> tempSwapchain;
	CHECK(config.factory->CreateSwapChainForHwnd(config.commandQueue, config.windowHandle, &swapChainDesc, nullptr, nullptr, &tempSwapchain));

	tempSwapchain.As(&m_Swapchain);

	// Create Descriptor
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = config.frameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	CHECK(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));

	m_RtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	m_BackBuffers.resize(config.frameCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (size_t i = 0; i < config.frameCount; i++)
	{
		CHECK(m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i])));

		device->CreateRenderTargetView(m_BackBuffers[i].Get(), nullptr, rtvHandle);

		rtvHandle.Offset(1, m_RtvDescriptorSize);
	}

}

D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetCurrentRTV()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_RTVHeap->GetCPUDescriptorHandleForHeapStart(),
		m_Swapchain->GetCurrentBackBufferIndex(),
		m_RtvDescriptorSize
	);
}

